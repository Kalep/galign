#include "Model1.hpp"
#include "Utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/random.hpp>
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
    for (size_t i = 0; i < sentence->src.size(); i++) {
      int tgtWord = dist(generator);
      jointCounts[sentence->src[i]][sentence->tgt[tgtWord]]++;
      sentence->align.push_back(tgtWord);
    }
    for (size_t i = 0; i < sentence->tgt.size(); i++)
      counts[sentence->tgt[i]]++;
  }
}

void Model1::RunIteration(bool doAggregate)
{
  vector<Sentence *> &sentences = corpus->GetSentences();
  random_shuffle(order.begin(), order.end());

  // over all words in corpus (in random order)
  #pragma omp parallel for
  for (size_t posIt = 0; posIt < order.size(); posIt++) {
    pair<size_t, size_t> sentPos = order[posIt];
    Sentence *sentence = sentences[sentPos.first];
    size_t srcWord = sentence->src[sentPos.second];
    size_t oldTgtWord = sentence->tgt[sentence->align[sentPos.second]];
    
    // discount removed alignment link
    if (--jointCounts[srcWord][oldTgtWord] <= 0) jointCounts[srcWord].Erase(oldTgtWord);
    counts[oldTgtWord]--;

    // calculate distribution parameters
    LogDistribution lexicalProbs;
    BOOST_FOREACH(size_t tgt, sentence->tgt) {
      float pairAlpha = alpha;
      float normAlpha = alpha * corpus->GetTotalSourceTypes();
      if (srcWord == tgt)
        pairAlpha = cognateAlpha;
      if (corpus->HasCognate(tgt))
        normAlpha += cognateAlpha - alpha;
      float logProb = log(jointCounts[srcWord][tgt] + pairAlpha) - log(counts[tgt] + normAlpha);
      lexicalProbs.Add(logProb);
    }
    lexicalProbs.Normalize();
    vector<float> distParams = lexicalProbs.Exp();

    // generate a sample
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
  int lineNum = 0;
  // aligns each word to the most probable counterpart
  // (cummulated counts from previous iterations are used to estimate
  // lexical probabilities)
  BOOST_FOREACH(Sentence *sentence, corpus->GetSentences()) {
    lineNum++;
    AlignmentType aggregAlign(sentence->src.size());
    for (size_t i = 0; i < sentence->src.size(); i++) {
      int best = -1;
      float bestProb = -numeric_limits<float>::infinity();
      for (size_t j = 0; j < sentence->tgt.size(); j++) {
        float pairAlpha = alpha;
        float normAlpha = alpha * corpus->GetTotalSourceTokens();
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
      if (best == -1) {
        Die("Zero probability for word '" + corpus->GetSrcWord(sentence->src[i]) +
            "' in sentence " + lexical_cast<string>(lineNum));
      }
      aggregAlign[i] = best;
    }
    out.push_back(aggregAlign);
  }
  return out;
}
