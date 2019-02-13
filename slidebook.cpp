#include "slidebook.h"

Slidebook::Slidebook(const std::string& filename)
{
    this->sb_read_file = III_NewSBReadFile(filename.c_str(), III::kNoExceptionsMasked);
    fmt::print("sb file loaded\n");

    this->number_captures = this->sb_read_file->GetNumCaptures();
    fmt::print("captures: {}\n", this->number_captures);
}

Slidebook::~Slidebook(){
    III_DeleteSBReadFile(this->sb_read_file);
}


CaptureDataFrame Slidebook::getDataFrame(int captureIndex){
    CaptureDataFrame cp = CaptureDataFrame(sb_read_file, captureIndex, 0);
    return cp;
}
