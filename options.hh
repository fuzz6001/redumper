#pragma once



#include <list>
#include <memory>
#include <string>



namespace gpsxre
{

struct Options
{
    std::string command;

    std::list<std::string> positional;
    
    bool help;
    bool verbose;

    std::string image_path;
    std::string image_name;
    bool overwrite;
    bool force_split;
    bool leave_unchanged;
    bool unsupported;

    std::string drive;
    std::unique_ptr<int> speed;
    int retries;
    bool refine_subchannel;
    std::unique_ptr<int> stop_lba;
    bool force_toc;
    bool force_qtoc;
    std::string skip;
    int skip_fill;
    int skip_size;
    int ring_size;
    bool iso9660_trim;
    bool skip_leadin;
    bool cdi_correct_offset;
    bool cdi_ready_normalize;
    bool descramble_new;
    std::unique_ptr<int> force_offset;
    int audio_silence_threshold;

    Options(int argc, const char *argv[]);

    static std::string HelpKeys();
    void PrintUsage();
};

}
