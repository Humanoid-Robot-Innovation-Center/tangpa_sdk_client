#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

constexpr const char* kDataDir = "data/";

template <typename T>
bool ReadProtoFromTextFile(const std::string& filename, T& message) {
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    std::cerr << "Failed to open pb.txt file: " << filename << std::endl;
    return false;
  }
  google::protobuf::io::IstreamInputStream stream(&ifs);
  if (!google::protobuf::TextFormat::Parse(&stream, &message)) {
    std::cerr << "Failed to parse pb.txt file: " << filename << std::endl;
    return false;
  }
  std::cout << "  [✓] Loaded parameters from " << filename << std::endl;
  return true;
}
