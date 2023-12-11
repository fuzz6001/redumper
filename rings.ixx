module;
#include <algorithm>
#include <memory>
#include <vector>
#include "throw_line.hh"

export module rings;

import cd.cdrom;
import dump;
import filesystem.iso9660;
import options;
import readers.disc_read_form1_reader;
import readers.sector_reader;
import scsi.cmd;
import utils.logger;
import utils.misc;



namespace gpsxre
{

export void redumper_rings(Context &ctx, Options &options)
{
	std::vector<uint8_t> toc_buffer = cmd_read_toc(*ctx.sptd);
	std::vector<uint8_t> full_toc_buffer = cmd_read_full_toc(*ctx.sptd);
	auto toc = choose_toc(toc_buffer, full_toc_buffer);

//	uint32_t sectors_count = std::filesystem::file_size(image_path) / (iso ? FORM1_DATA_SIZE : CD_DATA_SIZE);

	std::vector<iso9660::Area> area_map;

	for(auto &s : toc.sessions)
	{
		for(uint32_t i = 0; i + 1 < s.tracks.size(); ++i)
		{
			auto &t = s.tracks[i];

			if(!(t.control & (uint8_t)ChannelQ::Control::DATA) || t.track_number == bcd_decode(CD_LEADOUT_TRACK_NUMBER) || t.indices.empty())
				continue;

			std::unique_ptr<SectorReader> sector_reader = std::make_unique<Disc_READ_Reader>(*ctx.sptd, t.indices.front());

			auto am = iso9660::area_map(sector_reader.get(), t.indices.front(), s.tracks[i + 1].lba_start - t.indices.front());
			area_map.insert(area_map.end(), am.begin(), am.end());
		}
	}

	if(area_map.empty())
		return;

	std::for_each(area_map.cbegin(), area_map.cend(), [](const iso9660::Area &area)
	{
		auto sectors_count = scale_up(area.size, FORM1_DATA_SIZE);
		LOG("LBA: [{:6} .. {:6}], count: {:6}, type: {}{}",
			area.offset, area.offset + sectors_count - 1, sectors_count, enum_to_string(area.type, iso9660::AREA_TYPE_STRING),
			area.name.empty() ? "" : std::format(", name: {}", area.name));
	});

	LOG("");
}

}
