#ifndef SLIDEBOOKLOADER_H
#define SLIDEBOOKLOADER_H
#include "SBReadFile.h"
#include "capturedataframe.h"
#include "fmt/format.h"
class Slidebook
{
private:
    III::SBReadFile * sb_read_file;

public:
    CaptureIndex number_captures;

public:
    /** contructor && destructor **/
    Slidebook(const std::string& fileName);
    ~Slidebook();

    /**
     * @brief get a data frame
     * @param captureIndex
     * @return
     */
    CaptureDataFrame getDataFrame(int captureIndex);

};

#endif // SLIDEBOOKLOADER_H
