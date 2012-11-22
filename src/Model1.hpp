#ifndef MODEL1_HPP_
#define MODEL1_HPP_

#include <algorithm>
#include <string>
#include <vector>

#include <boost/random.hpp>

#include "Corpus.hpp"
#include "Utils.hpp"

typedef SafeHash<size_t, int>  CountType;
typedef SafeHash<size_t, CountType> JointCountType;

class Model1
{
public:
  Model1(Corpus *corpus, float alpha, float cognateAlpha) :
    corpus(corpus), alpha(alpha), cognateAlpha(cognateAlpha)
  {
    order = corpus->GetTokensToSentences();
  }
  void AlignRandomly();
  void RunIteration(bool doAggregate);
  std::vector<AlignmentType> GetAggregateAlignment();
  
  const CountType &GetCounts()           { return counts; }
  const JointCountType &GetJointCounts() { return jointCounts; }
private:
  boost::random::mt19937 generator;
  SentenceMappingType order;
  Corpus *corpus;
  JointCountType jointCounts, aggregateJoint; 
  CountType counts, aggregateCounts;
  float alpha, cognateAlpha;
};

#endif // MODEL1_HPP_
