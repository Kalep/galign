#ifndef CORPUS_HPP_
#define CORPUS_HPP_

#include <iostream>
#include <string>
#include <vector>

#include <boost/iostreams/filtering_stream.hpp> 
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

typedef std::vector<int> AlignmentType;
typedef std::vector<std::pair<size_t, size_t> > SentenceMappingType;

struct Sentence
{
  std::vector<std::string> src;
  std::vector<std::string> tgt;
  AlignmentType align;
};

class Corpus
{
public:
  // filename can be empty, corpus is then read from stdin
  Corpus(const std::string &fileName);

  std::vector<Sentence *> &GetSentences() { return sentences; }
  const SentenceMappingType &GetTokensToSentences() { return tokensToSentences; }
  const boost::unordered_set<std::string> &GetSrcTypes() { return sourceTypes; }
  size_t GetTotalSourceTokens() { return totalSourceTokens; }
  bool HasCognate(const std::string &word) { return cognates.find(word) != cognates.end(); }

private:
  void Read(boost::iostreams::filtering_istream &in);

  std::vector<Sentence *> sentences;
  SentenceMappingType tokensToSentences; // for random shuffling
  boost::unordered_set<std::string> sourceTypes;
  boost::unordered_set<std::string> cognates;
  size_t totalSourceTokens;
};

#endif // CORPUS_HPP_

