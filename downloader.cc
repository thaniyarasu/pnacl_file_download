#include <sstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/cpp/directory_entry.h"
#include "ppapi/cpp/file_io.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/file_system.h"

#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/cpp/url_response_info.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "downloader.h"

Downloader::Downloader(pp::Instance* instance,std::string* files,int files_size,int buffer_size) : 
  instance_(instance),  callback_factory_(this),
  file_system_(instance, PP_FILESYSTEMTYPE_LOCALPERSISTENT),
  file_thread_(instance),  file_system_ready_(false),  loader_(instance),
  buffer_(new char[buffer_size]),  Files(files), BufferSize(buffer_size),  
  FilesSize(files_size), DownloadedSize(0),  isDone(0)  {
    file_thread_.Start();
    file_thread_.message_loop().PostWork(
    callback_factory_.NewCallback(&Downloader::OpenFileSystem));
}

void Downloader::Start(){
  if (FilesSize==DownloadedSize) {
    isDone = 1;
    //Parent->DownloadDone();
    return;
  }
  std::string name = Files[DownloadedSize];
  printf("downloading %d : %s\n",DownloadedSize,name.c_str());
  DownloadedSize += 1;
  content_.clear();
  pp::URLRequestInfo url_request_(instance_);
  url_request_.SetURL(name);
  url_request_.SetMethod("GET");
  //url_request_.SetStreamToFile(true);
  loader_ = pp::URLLoader(instance_);
  loader_.Open(url_request_, callback_factory_.NewCallback(&Downloader::Opened,name));
}

void Downloader::Opened(int32_t result,std::string name) {
  if (result != PP_OK) {
    logmsg("URL could not be requested");
    return;
  }
  response_ = loader_.GetResponseInfo();
  Read(name);
}  

void Downloader::Read(std::string name){
  pp::CompletionCallback cc = callback_factory_.NewOptionalCallback(&Downloader::Readed,name);
  int32_t result = PP_OK;
  do {
    result = loader_.ReadResponseBody(buffer_, BufferSize, cc);
    if (result > 0) content_.append(buffer_, result);
  } while (result > 0);
  if (result != PP_OK_COMPLETIONPENDING) {
    cc.Run(result);
  }
}
  
void Downloader::Readed(int32_t result,std::string name) {
  if (result == PP_OK) {
    file_thread_.message_loop().PostWork(callback_factory_.NewCallback(
          &Downloader::Save, name, content_));
  } else if (result > 0) {
    content_.append(buffer_, result);
    Read(name);
  } else {
    logmsg("A read error occurred");
  }
}

void Downloader::Save(int32_t  result , const std::string& file_name,
            const std::string& file_contents) {
  pp::FileRef ref(file_system_, file_name.c_str());
  pp::FileIO file(instance_);
  int32_t open_result = file.Open(ref,
                  PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_CREATE |
                      PP_FILEOPENFLAG_TRUNCATE,
                  pp::BlockUntilComplete());
  if (open_result != PP_OK) {
    ShowErrorMessage("File open for write failed", open_result);
    return;
  }
  if (!file_contents.empty()) {
    if (file_contents.length() > INT32_MAX) {
      ShowErrorMessage("File too big", PP_ERROR_FILETOOBIG);
      return;
    }
    int64_t offset = 0;
    int32_t bytes_written = 0;
    do {
      bytes_written = file.Write(offset, file_contents.data() + offset,
        file_contents.length(),  pp::BlockUntilComplete());
      if (bytes_written > 0) {
        offset += bytes_written;
      } else {
        ShowErrorMessage("File write failed", bytes_written);
        return;
      }
    } while (bytes_written < static_cast<int64_t>(file_contents.length()));
  }
  int32_t flush_result = file.Flush(pp::BlockUntilComplete());
  if (flush_result != PP_OK) {
    ShowErrorMessage("File fail to flush", flush_result);
    return;
  }
  ShowStatusMessage("Save success : "+file_name);
  Start();
}

void Downloader::OpenFileSystem(int32_t ) {
  int32_t rv = file_system_.Open(1024 * 1024, pp::BlockUntilComplete());
  if (rv == PP_OK) {
    file_system_ready_ = true;
  } else {
    logmsg("Failed to open file system");
  }
}

void Downloader::ShowErrorMessage(const std::string& message, int32_t result) {
  std::stringstream ss;
  ss << "ERR|" << message << " -- Error #: " << result;
  printf("%s\n",ss.str().c_str());
}
void Downloader::ShowStatusMessage(const std::string& message) {
  std::stringstream ss;
  ss << "STAT|" << message;
  printf("%s\n",ss.str().c_str());
}
void Downloader::logmsg(const char* pMsg){
  fprintf(stdout,"logmsg: %s\n",pMsg);
}
