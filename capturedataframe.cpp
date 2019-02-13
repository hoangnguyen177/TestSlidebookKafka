#include "capturedataframe.h"

CaptureDataFrame::CaptureDataFrame(III::SBReadFile * sb_read_file, CaptureIndex capture_index, PositionIndex position_index)
{
    this->sb_read_file = sb_read_file;
    this->capture_index = capture_index;
    this->position_index = position_index;

    this->number_captures = this->sb_read_file->GetNumCaptures();
    this->number_channels = this->sb_read_file->GetNumChannels(capture_index);
    this->number_positions = this->sb_read_file->GetNumPositions(capture_index);
    this->number_timepoints = this->sb_read_file->GetNumTimepoints(capture_index);

    this->xDim = this->sb_read_file->GetNumXColumns(capture_index);
    this->yDim = this->sb_read_file->GetNumYRows(capture_index);
    this->zDim = this->sb_read_file->GetNumZPlanes(capture_index);

    this->image_name = CaptureDataFrame::GetString(sb_read_file, capture_index, &III::SBReadFile::GetImageName);
    this->image_comments = CaptureDataFrame::GetString(sb_read_file, capture_index, &III::SBReadFile::GetImageComments);
    this->capture_date =CaptureDataFrame::GetString(sb_read_file, capture_index, &III::SBReadFile::GetCaptureDate);
    this->lens_name = CaptureDataFrame::GetString(sb_read_file, capture_index, &III::SBReadFile::GetLensName);

    has_voxel_size = sb_read_file->GetVoxelSize(capture_index, voxel_size[0], voxel_size[1], voxel_size[2]);
    if (!has_voxel_size)
    {
        voxel_size[0] = voxel_size[1] = voxel_size[2] = 1.0;
    }

    for (int i = 0; i < number_channels; i++)
    {
        channel_names.push_back(GetString(sb_read_file, capture_index, i, &III::SBReadFile::GetChannelName));
        exposure_time.push_back(sb_read_file->GetExposureTime(capture_index, i));
    }
    // create the buffer
    std::size_t planeSize = this->xDim * this->yDim;
    int bufferSize = this->xDim * this->yDim * this->zDim;
    fmt::print("buffer size: {} \n", bufferSize);
    buffer = new UInt16[bufferSize];

    bufferSizeInBytes = bufferSize*sizeof(UInt16);


    int cappedTime = this->number_timepoints;
    /*
    if (options.max_time > -1)
    {
        cappedTime = std::min(cappedTime, options.max_time);
    }
    */

    for (int timepoint_index = 0; timepoint_index < cappedTime; timepoint_index++)
    {
        this->timepoint_index = timepoint_index;
        for (int c = 0; c < this->number_channels; c++)
        {
            this->channels_index = c;
            for (int z = 0; z < this->zDim; z++)
            {
                sb_read_file->ReadImagePlaneBuf(buffer + (z * planeSize), capture_index, 0, timepoint_index, z, c);
            }
            fmt::print("read buffer capture: {} time: {} channel: {}\n", capture_index, timepoint_index, c);
        }
    }
}

CaptureDataFrame::~CaptureDataFrame(){
    delete this->buffer;
}

std::string CaptureDataFrame::GetHeader(int capture_index, int position_index)
{
    return fmt::format("capture {} of {} : position {} of {}, time points: {}, channels: {}",
        capture_index + 1, number_captures, position_index + 1, number_positions,
        number_timepoints, number_channels);
}

std::string CaptureDataFrame::GetDetail()
{
    std::string metaData;
    metaData += fmt::format("Image name: {}\n", image_name);
    metaData +=  fmt::format("Image size: [{},{},{}]\n", xDim, yDim, zDim);
    std::string voxel_status;
    if (!has_voxel_size)
    {
        voxel_status = "undefined defaulting ";
    }
    metaData += fmt::format("Voxel size: {}[{},{},{}]\n", voxel_status, voxel_size[0], voxel_size[1], voxel_size[2]);
    metaData += fmt::format("Image comments: {}\n", image_comments);
    metaData += fmt::format("Capture date: {}\n", capture_date);
    metaData += fmt::format("Lens name: {}\n", lens_name);

    if (number_channels == 1)
    {
        metaData += fmt::format("Channel name: {}\n", channel_names[0]);
        metaData += fmt::format("Channel exposure time: {}ms\n", exposure_time[0]);
    }
    else
    {
        util::RangePrinter<1> channelRange(number_channels);
        for (int c = 0; c < number_channels; c++)
        {
            metaData += fmt::format("Channel {}\n   name: {}\n   exposure time: {}ms\n",
                channelRange.string(c),
                channel_names[c],
                exposure_time[c]);
        }
    }
    return metaData;
}

