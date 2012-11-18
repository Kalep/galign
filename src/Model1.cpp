#include "Model1.hpp"
#include "Utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/random.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/foreach.hpp>
#include <cmath>

using namespace std;
using namespace boost;
using namespace boost::random;

void Model1::AlignRandomly()
{
  vector<Sentence *> &sentences = corpus->GetSentences();
  BOOST_FOREACH(Sentence *sentence, sentences) {
    if (! sentence->align.empty())
      Die("Attempted to overwrite existing alignment with random initialization");
    sentence->align.reserve(sentence->src.size());

    // align and collect counts
    uniform_int_distribution<int> dist(0, sentence->tgt.size() - 1);
    for (int i = 0; i < sentence->src.size(); i++) {
      int tgtWord = dist(generator);
      jointCounts[sentence->src[i]][sentence->tgt[tgtWord]]++;
      sentence->align.push_back(tgtWord);
    }
    for (int i = 0; i < sentence->tgt.size(); i++)
      counts[sentence->tgt[i]]++;
  }
}

void Model1::RunIteration(bool doAggregate)
{
  vector<int> order;
  order.reserve(corpus->GetTotalSourceTokens());
  for (int i = 0; i < corpus->GetTotalSourceTokens(); i++)
    order.push_back(i);

  vector<Sentence *> &sentences = corpus->GetSentences();
  random_shuffle(order.begin(), order.end());

  // over all words in corpus (in random order)
  for (size_t i = 0; i < order.size(); i++) {
    int position = order[i];
    pair<int, int> sentPos = corpus->GetSentenceAndPosition(position);
    Sentence *sentence = sentences[sentPos.first];
    const string &srcWord = sentence->src[sentPos.second];
    const string &prevTgtWord = sentence->tgt[sentence->align[sentPos.second]];
    
    // discount removed alignment link
    if (--jointCounts[srcWord][prevTgtWord] <= 0) jointCounts[srcWord].erase(prevTgtWord);
    counts[prevTgtWord]--;

    // generate a sample
    LogDistribution lexicalProbs;
    BOOST_FOREACH(string tgt, sentence->tgt) {
      float pairAlpha = alpha;
      float normAlpha = alpha * corpus->GetSrcTypes().size();
      if (srcWord == tgt)
        pairAlpha = cognateAlpha;
      if (corpus->HasCognate(tgt))
        normAlpha += cognateAlpha - alpha;

      float logProb = log(jointCounts[srcWord][tgt] + pairAlpha) - log(counts[tgt] + normAlpha);
      lexicalProbs.Add(logProb);
    }
    lexicalProbs.Normalize();
    vector<float> distParams = lexicalProbs.Exp();
    discrete_distribution<int> dist(distParams.begin(), distParams.end());
    int sample = dist(generator);

    // update counts with new alignment link
    sentence->align[sentPos.second] = sample;
    jointCounts[srcWord][sentence->tgt[sample]]++;
    counts[sentence->tgt[sample]]++;
    if (doAggregate) {
      aggregateJoint[srcWord][sentence->tgt[sample]]++;
      aggregateCounts[sentence->tgt[sample]]++;
    }
  }
}

vector<AlignmentType> Model1::GetAggregateAlignment()
{
  vector<AlignmentType> out;
  BOOST_FOREACH(Sentence *sentence, corpus->GetSentences()) {
    AlignmentType aggregAlign(sentence->src.size());
    for (int i = 0; i < sentence->src.size(); i++) {
      int best = -1;
      float bestProb = numeric_limits<float>::min();
      for (int j = 0; j < sentence->tgt.size(); j++) {
        float pairAlpha = alpha;
        float normAlpha = alpha * corpus->GetSrcTypes().size();
        if (sentence->src[i] == sentence->tgt[j])
          pairAlpha = cognateAlpha;
        if (corpus->HasCognate(sentence->tgt[j]))
          normAlpha += cognateAlpha - alpha;

        float logProb = log(aggregateJoint[sentence->src[i]][sentence->tgt[j]] + pairAlpha)
          - log(aggregateCounts[sentence->tgt[j]] + normAlpha);
        if (logProb > bestProb) {
          best = j;
          bestProb = logProb;
        }      
      }
      aggregAlign[i] = best;
    }
    out.push_back(aggregAlign);
  }
  return out;
}
