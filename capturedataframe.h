#ifndef CAPTUREDATAFRAME_H
#define CAPTUREDATAFRAME_H

#include <math.h>
#include <utility>
#include <unordered_set>
#include <string>
#include <vector>
#include "SBReadFile.h"
#include "fmt/format.h"


namespace util
{
    template <int N = 0>
    class RangePrinter
    {
    public:
        int max;
        int numDigits;
        RangePrinter() {}
        RangePrinter(int m) : numDigits(m + N == 0 ? 1 : (int)(floor(log10(m + N)) + 1)) {}
        std::string string(int v) const
        {
            return fmt::format("{:0{}}", v + N, numDigits);
        }
    };
}

class CaptureDataFrame
{
public:
    III::SBReadFile * sb_read_file;

    CaptureIndex number_captures;
    PositionIndex number_positions;
    ChannelIndex number_channels;
    TimepointIndex number_timepoints;

    CaptureIndex capture_index;
    PositionIndex position_index;
    ChannelIndex channels_index;
    TimepointIndex timepoint_index;

    SInt32 xDim;
    SInt32 yDim;
    SInt32 zDim;

    bool has_voxel_size = false;
    float voxel_size[3];
    std::string image_name;
    std::string image_comments;
    std::string capture_date;
    std::string lens_name;
    std::vector<std::string> channel_names;
    std::vector<SInt32> exposure_time;

    //buffer
    UInt16 * buffer;
    int bufferSizeInBytes;

public:
    /** constructor **/
    CaptureDataFrame(III::SBReadFile * sb_read_file, CaptureIndex capture_index, PositionIndex position_index);
    /** destructor **/
    ~CaptureDataFrame();
    /** header **/
    std::string GetHeader(int capture_index, int position_index);
    /** detail **/
    std::string GetDetail();


    /** static methods **/
    static std::string GetString(III::SBReadFile * sb_read_file, int capture_index, UInt32(III::SBReadFile::*sb_string_read)(char *, const CaptureIndex ci) const)
    {
        std::string sb_data;
        int char_count = (sb_read_file->*sb_string_read)(nullptr, capture_index);
        if (char_count > 0)
        {
            char * cbuff = new char[char_count];
            (sb_read_file->*sb_string_read)(cbuff, capture_index);
            sb_data = std::string(cbuff);
            delete[] cbuff;
        }
        return sb_data;
    }

    static std::string GetString(III::SBReadFile * sb_read_file, int capture_index, int channel_index, UInt32(III::SBReadFile::*sb_string_read)(char *, const CaptureIndex, const ChannelIndex) const)
    {
        std::string sb_data;
        int char_count = (sb_read_file->*sb_string_read)(nullptr, capture_index, channel_index);
        if (char_count > 0)
        {
            char * cbuff = new char[char_count];
            (sb_read_file->*sb_string_read)(cbuff, capture_index, channel_index);
            sb_data = std::string(cbuff);
            delete[] cbuff;
        }
        return sb_data;
    }

};

#endif // CAPTUREDATAFRAME_H
