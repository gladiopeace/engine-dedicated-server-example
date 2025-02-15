// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_DOWNLOADER_H_
#define SRC_FUNAPI_DOWNLOADER_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiDownloadFileInfo;
class FunapiHttpDownloaderImpl;
class FUNAPI_API FunapiHttpDownloader : public std::enable_shared_from_this<FunapiHttpDownloader> {
 public:
  enum class ResultCode : int {
    kNone,
    kSucceed,
    kFailed,
  };

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const fun::vector<std::shared_ptr<FunapiDownloadFileInfo>>&)> ReadyHandler;

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const fun::vector<std::shared_ptr<FunapiDownloadFileInfo>>&,
                             const int index,
                             const int max_index,
                             const uint64_t received_bytes,
                             const uint64_t expected_bytes)> ProgressHandler;

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const fun::vector<std::shared_ptr<FunapiDownloadFileInfo>>&,
                             const ResultCode)> CompletionHandler;

  FunapiHttpDownloader() = delete;
  FunapiHttpDownloader(const fun::string &url, const fun::string &path);
  virtual ~FunapiHttpDownloader();

  static std::shared_ptr<FunapiHttpDownloader> Create(const fun::string &url, const fun::string &path);

  void AddReadyCallback(const ReadyHandler &handler);
  void AddProgressCallback(const ProgressHandler &handler);
  void AddCompletionCallback(const CompletionHandler &handler);
  void SetTimeoutPerFile(long timeout_in_seconds);
  void SetCACertFilePath(const fun::string &path);


  void Start();
  void Start(const fun::string &inclusive_path);
  void Update();
  static void UpdateAll();

 private:
  std::shared_ptr<FunapiHttpDownloaderImpl> impl_;
};


class FunapiDownloadFileInfoImpl;
class FUNAPI_API FunapiDownloadFileInfo : public std::enable_shared_from_this<FunapiDownloadFileInfo> {
 public:
  FunapiDownloadFileInfo() = delete;
  FunapiDownloadFileInfo(const fun::string &url,
                         const fun::string &path,
                         const uint64_t size,
                         const fun::string &hash,
                         const fun::string &hash_front);
  virtual ~FunapiDownloadFileInfo();

  const fun::string& GetUrl();
  const fun::string& GetPath();
  uint64_t GetSize();
  const fun::string& GetHash();
  const fun::string& GetHashFront();

  const FunapiHttpDownloader::ResultCode GetResultCode();
  void SetResultCode(FunapiHttpDownloader::ResultCode r);

 private:
  std::shared_ptr<FunapiDownloadFileInfoImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_DOWNLOADER_H_
