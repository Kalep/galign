#include "Utils.hpp"

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp> 
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp> 
#include <boost/algorithm/string/predicate.hpp>

using namespace boost::algorithm;
using namespace boost::iostreams;
using namespace boost;
using namespace std;

filtering_istream *InitInput(const string &fileName)
{
  filtering_istream *in = new filtering_istream();
  if (fileName.empty()) {
    in->push(cin);
  } else {
    if (ends_with(fileName, ".gz"))
      in->push(gzip_decompressor());
    file_source fileSrc(fileName.c_str());
    if (! fileSrc.is_open())
      Die("Cannot read file: " + fileName);
    in->push(fileSrc);
  }
  return in;
}

filtering_ostream *InitOutput(const string &fileName)
{
  filtering_ostream *out = new filtering_ostream();
  if (fileName.empty()) {
    out->push(cout);
  } else {
    if (ends_with(fileName, ".gz"))
      out->push(gzip_compressor());
    file_sink fileSink(fileName.c_str());
    if (! fileSink.is_open())
      Die("Cannot write to file: " + fileName);
    out->push(fileSink);
  }
  return out;
}

