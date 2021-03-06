#include "Writer.hpp"

#include <boost/iostreams/filtering_stream.hpp> 
#include <boost/foreach.hpp>

using namespace std;
using namespace boost::iostreams;

void Writer::WriteAlignment(const std::string &fileName, bool gizaFormat)
{
  filtering_ostream *out = InitOutput(fileName);
  BOOST_FOREACH(Sentence *sentence, corpus->GetSentences()) {
    WriteAlignmentLine(*out, sentence->src, sentence->tgt, sentence->align, gizaFormat);  
  }
  close(*out);
}

void Writer::WriteAlignment(const std::string &fileName, const vector<AlignmentType> &align,
    bool gizaFormat)
{
  filtering_ostream *out = InitOutput(fileName);
  vector<AlignmentType>::const_iterator alignIt = align.begin();
  BOOST_FOREACH(Sentence *sentence, corpus->GetSentences()) {
    WriteAlignmentLine(*out, sentence->src, sentence->tgt, *(alignIt++), gizaFormat);
  }
  close(*out);
}

void Writer::WriteAlignmentLine(filtering_ostream &out, const WordSequenceType &src,
  const WordSequenceType &tgt, const AlignmentType &align, bool gizaFormat)
{
  for (size_t i = 0; i < src.size(); i++) {
    if (align[i] != 0) {
      if (gizaFormat) {
        out << i << "-" << align[i];
      } else {
        out << corpus->GetSrcWord(src[i]) << "{" << corpus->GetTgtWord(tgt[align[i]]) << "}";
      }
      if (i != src.size() - 1)
        out << " ";
    }
  }
  out << "\n";
}
