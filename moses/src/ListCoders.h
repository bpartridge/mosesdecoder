#ifndef LISTCODERS_H__
#define LISTCODERS_H__

#include <cmath>

namespace Moses {

template <typename T = unsigned int>
class VarIntType {
  private:
    template <typename IntType, typename OutIt>
    static void encodeSymbol(IntType input, OutIt output) {
        if(input == 0) {
          *output = 0;
          output++;
          return;
        }
      
        T msb = 1 << (sizeof(T)*8-1);
        IntType mask  = ~msb;
        IntType shift = (sizeof(T)*8-1);
        
        while(input) {
            T res = input & mask;
            input >>= shift;
            if(input)
                res |= msb;
            *output = res;
            output++;
        }
    };
    
    template <typename InIt, typename IntType>
    static void decodeSymbol(InIt &it, InIt end, IntType &output) {
        T msb = 1 << (sizeof(T)*8-1);
        IntType shift = (sizeof(T)*8-1);
        
        output = 0;
        size_t i = 0;
        while(it != end && *it & msb) {
            IntType temp = *it & ~msb;
            temp <<= shift*i;
            output |= temp;
            it++; i++;
        }
        assert(it != end);
        
        IntType temp = *it;
        temp <<= shift*i;
        output |= temp;
        it++;
    }
    
    

  public:
        
    template <typename InIt, typename OutIt>
    static void encode(InIt it, InIt end, OutIt outIt) {
        while(it != end) {
            encodeSymbol(*it, outIt);
            it++;
        }
    }
    
    
    template <typename InIt, typename OutIt>
    static void decode(InIt &it, InIt end, OutIt outIt) {
        while(it != end) {
            size_t output;
            decodeSymbol(it, end, output);
            *outIt = output;
            outIt++;
        }
    }
    
    template <typename InIt>
    static size_t decodeAndSum(InIt &it, InIt end, size_t num) {
        size_t sum = 0;
        size_t curr = 0;
        
        while(it != end && curr < num) {
          size_t output;
          decodeSymbol(it, end, output);
          sum += output; curr++;
        }
        
        return sum;
    }

};

typedef VarIntType<unsigned char> VarByte;

typedef VarByte VarInt8;
typedef VarIntType<unsigned short> VarInt16;
typedef VarIntType<unsigned int>   VarInt32;

class Simple9 {
  private:
    typedef unsigned int uint;
        
    template <typename InIt>
    inline static void encodeSymbol(uint &output, InIt it, InIt end) {
        uint length = end - it;
        uint bitlength = 28/length;
        output = 0;
        output |= (length << 28);

        uint i = 0;
        while(it != end) {
            uint l = bitlength * (length-i-1);
            output |= *it << l;
            it++;
            i++;
        }
    }
    
    template <typename OutIt>
    static void decodeSymbol(uint input, OutIt outIt) {
        uint length = (input >> 28);
        uint bitlength = 28/length;
        uint mask = (1 << bitlength)-1;
        uint shift = bitlength * (length-1);
        while(shift > 0) {
          *outIt = (input >> shift) & mask;
          shift -= bitlength;  
          outIt++;
        }
        *outIt = input & mask;
        outIt++;  
    }
    
    static size_t decodeAndSumSymbol(uint input, size_t num, size_t &curr) {
        uint length = (input >> 28);
        uint bitlength = 28/length;
        uint mask = (1 << bitlength)-1;
        
        uint shift = bitlength * (length-1);
        size_t sum = 0;
        
        while(shift > 0) {
          sum += (input >> shift) & mask;
          shift -= bitlength;
          if(++curr == num)
            return sum;
        }
        sum += input & mask;
        curr++;
        return sum;
    }
    
  public:
    template <typename InIt, typename OutIt>
    static void encode(InIt it, InIt end, OutIt outIt) {        
        uint parts[] = { 1, 2, 3, 4, 5, 7, 9, 14, 28 };
        uint bits[]  = { 0, 1, 2, 3, 4, 5, 7, 7, 9, 9, 14, 14, 14, 14, 14,
          28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28 };

        uint buffer[28];
        for(InIt i = it; i < end; i++) {
            uint lastbit = 1;
            uint lastpos = 0;
            uint lastyes = 0;
            uint j = 0;
            
            double log2 = log(2);
            while(j < 9 && lastpos < 28 && (i+lastpos) < end) {
                if(lastpos >= parts[j]) 
                    j++;
                
                buffer[lastpos] = *(i + lastpos);
                
                uint reqbit = ceil(log(buffer[lastpos]+1)/log2);
                assert(reqbit <= 28);
                
                uint bit = bits[reqbit];
                if(lastbit < bit)
                    lastbit = bit;
                    
                if(parts[j] > 28/lastbit)
                    break;
                else if(lastpos == parts[j]-1)
                    lastyes = lastpos;
                    
                lastpos++;
            }
            i += lastyes;       
            
            uint length = lastyes + 1;
            uint output;
            encodeSymbol(output, buffer, buffer + length);
            
            *outIt = output;
            outIt++;
        }
    }
    
    template <typename InIt, typename OutIt>
    static void decode(InIt &it, InIt end, OutIt outIt) {
        while(it != end) {
            decodeSymbol(*it, outIt);
            it++;
        }
    }
    
    template <typename InIt>
    static size_t decodeAndSum(InIt &it, InIt end, size_t num) {
        size_t sum = 0;
        size_t curr = 0;
        while(it != end && curr < num) {
            sum += decodeAndSumSymbol(*it, num, curr);
            it++;
        }
        assert(curr == num);
        return sum;
    }
};

}

#endif