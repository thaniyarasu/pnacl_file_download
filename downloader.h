#ifndef DOWNLOADER_H_
#define DOWNLOADER_H_

#include <string>
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/cpp/directory_entry.h"
#include "ppapi/cpp/file_io.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/file_system.h"


#define READ_BUFFER_SIZE 32768

class Downloader {
  public:
    void Start();
    Downloader(pp::Instance* instance,std::string* files,int files_size,int buffer_size);
  private:
    pp::Instance* instance_;  // Weak pointer.
    //FileIoInstance* Parent;
    pp::CompletionCallbackFactory<Downloader> callback_factory_;
    pp::FileSystem file_system_;
    pp::SimpleThread file_thread_;
    bool file_system_ready_;
    pp::URLLoader loader_;
    pp::URLResponseInfo response_;
    std::string content_;
    char* buffer_;  
    std::string* Files;
    int BufferSize;
    int FilesSize;// = sizeof(files)/sizeof(files[0]) ;
    int DownloadedSize;
    bool isDone;

    void Opened(int32_t result,std::string name);
    void Read(std::string name);
    void Readed(int32_t result,std::string name);
    void Save(int32_t , const std::string& ,const std::string& );
    void OpenFileSystem(int32_t);
    void logmsg(const char*);
    void ShowErrorMessage(const std::string& , int32_t );
    void ShowStatusMessage(const std::string&);
};

#endif  // DOWNLOADER_H_

