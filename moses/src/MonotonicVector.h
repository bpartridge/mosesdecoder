#ifndef MONOTONICVECTOR_H__
#define MONOTONICVECTOR_H__

// MonotonicVector - Represents a monotonic increasing function that maps
// positive integers of any size onto a given number type. Each value has to be
// equal or larger than the previous one. Depending on the stepSize it can save
// up to 90% of memory compared to a std::vector<long>. Time complexity is roughly
// constant, in the worst case, however, stepSize times slower than a normal
// std::vector.

#include <vector>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cassert>

#include "ListCoders.h"

namespace Moses {

template<typename PosT = size_t, typename NumT = size_t, PosT stepSize = 32>
class MonotonicVector {
  private:
    std::vector<NumT> m_anchors;
    std::vector<unsigned int> m_diffs;
    std::vector<unsigned int> m_tempDiffs;
    
    size_t m_size;
    PosT m_last;
    bool m_final;
    
  public:
    typedef PosT value_type;
    
    MonotonicVector() : m_size(0), m_last(0), m_final(false) {}
    
    size_t size() const {
      return m_size + m_tempDiffs.size();
    }
    
    PosT at(size_t i) const {
      PosT s = stepSize;
      PosT j = m_anchors[i / s];
      PosT r = i % s;
          
      std::vector<unsigned int>::const_iterator it = m_diffs.begin() + j;
      
      PosT k = 0;
      k += VarInt32::decodeAndSum(it, m_diffs.end(), 1);
      if(i < m_size)
        k += Simple9::decodeAndSum(it, m_diffs.end(), r);
      else if(i < m_size + m_tempDiffs.size())
        for(int l = 0; l < r; l++)
          k += m_tempDiffs[l];
      
      return k;
    }
    
    PosT operator[](PosT i) const {
      return at(i);
    }
    
    PosT back() const {
      return at(size()-1);
    }
    
    void push_back(PosT i) {
      assert(m_final != true);
    
      if(m_anchors.size() == 0 && m_tempDiffs.size() == 0) {
        m_anchors.push_back(0);
        VarInt32::encode(&i, &i+1, std::back_inserter(m_diffs));
        m_last = i;
        m_size++;
            
        return;
      }
      
      if(m_tempDiffs.size() == stepSize-1) {
        Simple9::encode(m_tempDiffs.begin(), m_tempDiffs.end(),
                        std::back_inserter(m_diffs));
        m_anchors.push_back(m_diffs.size());
        VarInt32::encode(&i, &i+1, std::back_inserter(m_diffs));
        
        m_size += m_tempDiffs.size() + 1;
        m_tempDiffs.clear();
      }
      else {
        PosT last = m_last;
        PosT diff = i - last;
        m_tempDiffs.push_back(diff);
      }
      m_last = i;
    }
    
    void commit() {
      assert(m_final != true);
      Simple9::encode(m_tempDiffs.begin(), m_tempDiffs.end(),
                      std::back_inserter(m_diffs));
      m_size += m_tempDiffs.size();
      m_tempDiffs.clear();
      m_final = true;
    }
    
    size_t usage() {      
      return m_diffs.size() * sizeof(unsigned int)
        + m_anchors.size() * sizeof(NumT);
    }
    
    size_t load(std::FILE* in) {
      size_t byteSize = 0;
      
      byteSize += fread(&m_final, sizeof(bool), 1, in) * sizeof(bool);
      byteSize += fread(&m_size, sizeof(size_t), 1, in) * sizeof(size_t);
      byteSize += fread(&m_last, sizeof(PosT), 1, in) * sizeof(PosT);
      
      size_t size;
      byteSize += fread(&size, sizeof(size_t), 1, in) * sizeof(size_t);
      m_diffs.resize(size);
      byteSize += fread(&m_diffs[0], sizeof(unsigned int), size, in) * sizeof(unsigned int);
      
      byteSize += fread(&size, sizeof(size_t), 1, in) * sizeof(size_t);
      m_anchors.resize(size);
      byteSize += fread(&m_anchors[0], sizeof(NumT), size, in) * sizeof(NumT);
      
      return byteSize;
    }
    
    size_t save(std::FILE* out) {
      if(!m_final)
        commit();
      
      bool byteSize = 0;
      byteSize += fwrite(&m_final, sizeof(bool), 1, out) * sizeof(bool);
      byteSize += fwrite(&m_size, sizeof(size_t), 1, out) * sizeof(size_t);
      byteSize += fwrite(&m_last, sizeof(PosT), 1, out) * sizeof(PosT);
      
      size_t size = m_diffs.size();
      byteSize += fwrite(&size, sizeof(size_t), 1, out) * sizeof(size_t);
      byteSize += fwrite(&m_diffs[0], sizeof(unsigned int), size, out) * sizeof(unsigned int);
      
      size = m_anchors.size();
      byteSize += fwrite(&size, sizeof(size_t), 1, out) * sizeof(size_t);
      byteSize += fwrite(&m_anchors[0], sizeof(NumT), size, out) * sizeof(NumT);
      
      return byteSize;
    }
    
    void swap(MonotonicVector<PosT, NumT, stepSize> &mv) {
      if(!m_final)
        commit();
        
      m_diffs.swap(mv.m_diffs);
      m_anchors.swap(mv.m_anchors);
    }
};

}
#endif
