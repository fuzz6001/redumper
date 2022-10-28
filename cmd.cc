#include "cd.hh"
#include "common.hh"
#include "endian.hh"
#include "mmc.hh"
#include "cmd.hh"



//DEBUG
#include <iostream>



namespace gpsxre
{

static const uint32_t READ_CD_C2_SIZES[] =
{
    0,
    CD_C2_SIZE,
    2 + CD_C2_SIZE,
    0
};

static const uint32_t READ_CD_SUB_SIZES[] =
{
	0,
	CD_SUBCODE_SIZE,
	16,
	0,
	CD_SUBCODE_SIZE,
	0,
	0,
	0
};

static const uint32_t READ_CDDA_SIZES[] =
{
	CD_DATA_SIZE,
	CD_DATA_SIZE + 16,
	CD_DATA_SIZE + CD_SUBCODE_SIZE,
	CD_SUBCODE_SIZE,
	0, //TODO: analyze other values
	0,
	0,
	0,
	CD_RAW_DATA_SIZE
};


SPTD::Status cmd_drive_ready(SPTD &sptd)
{
	CDB6_Generic cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::TEST_UNIT_READY;
	return sptd.SendCommand(&cdb, sizeof(cdb), nullptr, 0);
}


DriveQuery cmd_drive_query(SPTD &sptd)
{
	DriveQuery drive_query;

	CDB6_Inquiry3 cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::INQUIRY;
	cdb.allocation_length = sizeof(InquiryData);
	InquiryData inquiry_data;
	auto status = sptd.SendCommand(&cdb, sizeof(cdb), &inquiry_data, sizeof(inquiry_data));
	if(status.status_code)
		throw_line(fmt::format("unable to query drive info, SCSI ({})", SPTD::StatusMessage(status)));

	drive_query.vendor_id = normalize_string(std::string((char *)inquiry_data.vendor_id, sizeof(inquiry_data.vendor_id)));
	drive_query.product_id = normalize_string(std::string((char *)inquiry_data.product_id, sizeof(inquiry_data.product_id)));
	drive_query.product_revision_level = normalize_string(std::string((char *)inquiry_data.product_revision_level, sizeof(inquiry_data.product_revision_level)));
	drive_query.vendor_specific = normalize_string(std::string((char *)inquiry_data.vendor_specific, sizeof(inquiry_data.vendor_specific)));

	return drive_query;
}


std::vector<uint8_t> cmd_read_toc(SPTD &sptd)
{
	std::vector<uint8_t> toc;

	CDB10_ReadTOC cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::READ_TOC;
	cdb.format2 = (uint8_t)READ_TOC_ExFormat::TOC;
	cdb.starting_track = 1;

	// read TOC header first to get the full TOC size
	READ_TOC_Response toc_response;
	*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(sizeof(toc_response));
	auto status = sptd.SendCommand(&cdb, sizeof(cdb), &toc_response, sizeof(toc_response));
	if(!status.status_code)
	{
		uint16_t toc_buffer_size = sizeof(toc_response.data_length) + endian_swap(toc_response.data_length);
		toc.resize(round_up(toc_buffer_size, sizeof(uint32_t)));

		*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(toc_buffer_size);
		status = sptd.SendCommand(&cdb, sizeof(cdb), toc.data(), (uint32_t)toc.size());
		if(status.status_code)
			toc.clear();
		else
			toc.resize(toc_buffer_size);
	}

	return toc;
}


std::vector<uint8_t> cmd_read_full_toc(SPTD &sptd)
{
	std::vector<uint8_t> full_toc;

	CDB10_ReadTOC cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::READ_TOC;
	cdb.format2 = (uint8_t)READ_TOC_ExFormat::FULL_TOC;
	cdb.starting_track = 1;

	// read TOC header first to get the full TOC size
	READ_TOC_Response toc_response;
	*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(sizeof(toc_response));
	auto status = sptd.SendCommand(&cdb, sizeof(cdb), &toc_response, sizeof(toc_response));
	if(!status.status_code)
	{
		uint16_t toc_buffer_size = sizeof(toc_response.data_length) + endian_swap(toc_response.data_length);
		full_toc.resize(round_up(toc_buffer_size, sizeof(uint32_t)));

		*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(toc_buffer_size);
		status = sptd.SendCommand(&cdb, sizeof(cdb), full_toc.data(), (uint32_t)full_toc.size());
		if(status.status_code)
			full_toc.clear();
		else
			full_toc.resize(toc_buffer_size);
	}

	return full_toc;
}


SPTD::Status cmd_read_cd_text(SPTD &sptd, std::vector<uint8_t> &cd_text)
{
	SPTD::Status status;

	CDB10_ReadTOC cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::READ_TOC;
	cdb.format2 = (uint8_t)READ_TOC_ExFormat::CD_TEXT;

	// read CD-TEXT header first to get the full TOC size
	READ_TOC_Response toc_response;
	*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(sizeof(toc_response));
	status = sptd.SendCommand(&cdb, sizeof(cdb), &toc_response, sizeof(toc_response));
	if(!status.status_code)
	{
		uint16_t cd_text_buffer_size = sizeof(toc_response.data_length) + endian_swap(toc_response.data_length);
		if(cd_text_buffer_size > sizeof(toc_response))
		{
			cd_text.resize(round_up(cd_text_buffer_size, sizeof(uint32_t)));

			*(uint16_t *)cdb.allocation_length = endian_swap<uint16_t>(cd_text_buffer_size);

			status = sptd.SendCommand(&cdb, sizeof(cdb), cd_text.data(), (uint32_t)cd_text.size());
			if(!status.status_code)
				cd_text.resize(cd_text_buffer_size);
		}
	}

	return status;
}


SPTD::Status cmd_read_cd(SPTD &sptd, uint8_t *sector, int32_t start_lba, uint32_t transfer_length, READ_CD_ExpectedSectorType expected_sector_type, READ_CD_ErrorField error_field, READ_CD_SubChannel sub_channel)
{
	CDB12_ReadCD cdb = {};

	cdb.operation_code = (uint8_t)CDB_OperationCode::READ_CD;
	cdb.expected_sector_type = (uint8_t)expected_sector_type;
	*(int32_t *)cdb.starting_lba = endian_swap(start_lba);
	cdb.transfer_blocks[0] = ((uint8_t *)&transfer_length)[2];
	cdb.transfer_blocks[1] = ((uint8_t *)&transfer_length)[1];
	cdb.transfer_blocks[2] = ((uint8_t *)&transfer_length)[0];
	cdb.error_flags = (uint8_t)error_field;
	cdb.include_edc = 1;
	cdb.include_user_data = 1;
	cdb.header_code = (uint8_t)READ_CD_HeaderCode::ALL;
	cdb.include_sync_data = 1;
	cdb.sub_channel_selection = (uint8_t)sub_channel;

	return sptd.SendCommand(&cdb, sizeof(cdb), sector, CD_RAW_DATA_SIZE * transfer_length);
}


SPTD::Status cmd_read_cdda(SPTD &sptd, uint8_t *sector, int32_t start_lba, uint32_t transfer_length, READ_CDDA_SubCode sub_code)
{
	CDB12_ReadCDDA cdb = {};

	cdb.operation_code = (uint8_t)CDB_OperationCode::READ_CDDA;
	*(int32_t *)cdb.starting_lba = endian_swap(start_lba);
	*(uint32_t *)cdb.transfer_blocks = endian_swap(transfer_length);
	cdb.sub_code = (uint8_t)sub_code;

	return sptd.SendCommand(&cdb, sizeof(cdb), sector, CD_RAW_DATA_SIZE * transfer_length);
}


SPTD::Status cmd_plextor_reset(SPTD &sptd)
{
	CDB6_Generic cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::PLEXTOR_RESET;

	return sptd.SendCommand(&cdb, sizeof(cdb), nullptr, 0);
}


SPTD::Status cmd_synchronize_cache(SPTD &sptd)
{
	CDB6_Generic cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::SYNCHRONIZE_CACHE;

	return sptd.SendCommand(&cdb, sizeof(cdb), nullptr, 0);
}


SPTD::Status cmd_flush_drive_cache(SPTD &sptd, int32_t lba)
{
	CDB12_Read cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::READ12;
	cdb.force_unit_access = 1;
	*(int32_t *)cdb.starting_lba = endian_swap(lba);

	return sptd.SendCommand(&cdb, sizeof(cdb), nullptr, 0);
}


SPTD::Status cmd_set_cd_speed(SPTD &sptd, uint16_t speed)
{
	CDB12_SetCDSpeed cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::SET_CD_SPEED;
	*(uint16_t *)cdb.read_speed = endian_swap(speed);

	return sptd.SendCommand(&cdb, sizeof(cdb), nullptr, 0);
}


SPTD::Status cmd_asus_read_cache(SPTD &sptd, uint8_t *buffer, uint32_t offset, uint32_t size)
{
	CDB_ASUS_ReadCache cdb;
	cdb.operation_code = (uint8_t)CDB_OperationCode::ASUS_READ_CACHE;
	cdb.unknown = 6;
	cdb.offset = endian_swap(offset);
	cdb.size = endian_swap(size);

	return sptd.SendCommand(&cdb, sizeof(cdb), buffer, size);
}


//DEBUG
SPTD::Status cmd_get_configuration(SPTD &sptd)
{
	CDB10_GetConfiguration cdb = {};
	cdb.operation_code = (uint8_t)CDB_OperationCode::GET_CONFIGURATION;
	cdb.requested_type = (uint8_t)GET_CONFIGURATION_RequestedType::ALL;
	cdb.starting_feature_number = 0;

	uint16_t size = std::numeric_limits<uint16_t>::max();
	*(uint16_t *)cdb.allocation_length = endian_swap(size);
	std::vector<uint8_t> buffer(size);

	auto status = sptd.SendCommand(&cdb, sizeof(cdb), buffer.data(), buffer.size());

	auto feature_header = (GET_CONFIGURATION_FeatureHeader *)buffer.data();
	uint32_t fds_size = endian_swap(feature_header->data_length) - (sizeof(GET_CONFIGURATION_FeatureHeader) - sizeof(feature_header->data_length));
	uint8_t *fds_start = buffer.data() + sizeof(GET_CONFIGURATION_FeatureHeader);
	uint8_t *fds_end = fds_start + fds_size;

	//FIXME: support more than 0xFFFF size?

	for(auto fds = (GET_CONFIGURATION_FeatureDescriptor *)fds_start;
		(uint8_t *)fds + sizeof(GET_CONFIGURATION_FeatureDescriptor) <= fds_end &&
		(uint8_t *)fds + sizeof(GET_CONFIGURATION_FeatureDescriptor) + fds->additional_length <= fds_end; fds = (GET_CONFIGURATION_FeatureDescriptor *)((uint8_t *)fds + sizeof(GET_CONFIGURATION_FeatureDescriptor) + fds->additional_length))
	{
		std::cout << endian_swap(fds->feature_code) << std::endl;
	}

	return status;
}

}
