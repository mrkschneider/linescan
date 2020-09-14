#include <cxxtest/TestSuite.h>

#include <linescan.h>
#include <algorithm>

class LinescanTestSuite : public CxxTest::TestSuite {

  char* b;
  uint size = 128;

  char c = 'd'; // charcode 100
  uint64_t cmask = linescan_create_mask(c);
  linescan* r;

public:

  void setUp(){
    init_buffer();

    r = linescan_create(size);
  }

  void tearDown(){
    free(b);
    linescan_free(r);
  }

  void init_buffer(){
    b = (char*)calloc(size,sizeof(char));
    size_t idx = 0;
    while(true){
      for(size_t i=0;i<26;i++){
	b[idx] = (char)(i+97);
	idx++;
	if(idx == size) return;
      }
    }
  }

  void test_linescan_find(){
    b[size-1] = '\n';
    int rc = linescan_find(b,cmask,size,r);
    std::vector<size_t> offsets{0,3,29,55,81,107,127};
    std::vector<char> offset_chars{'a','d','d','d','d','d','\n'};

    // Normal operations
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(size,r->size);
    TS_ASSERT_EQUALS(b,r->buf);
    TS_ASSERT_EQUALS(7,r->offsets_n);
    TS_ASSERT_EQUALS(size,r->offsets_size);
    for(size_t i=0;i<r->offsets_n;i++){
      TS_ASSERT_EQUALS(offset_chars[i],r->buf[r->offsets[i]]);
    }
    TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n)); 
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 0); // Aligned, no steps necessary
    TS_ASSERT_EQUALS(r->debug_steps_2, 16); // 16 * 8 = 128
    TS_ASSERT_EQUALS(r->debug_steps_3, 8); // Newline in last position
#endif

    // Cannot find newline
    rc = linescan_find(b,cmask,size-1,r);
    TS_ASSERT_EQUALS(size-1,r->size);
    TS_ASSERT_EQUALS(0,rc);
    TS_ASSERT_EQUALS(6,r->offsets_n);
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 0); // Aligned, no steps necessary
    TS_ASSERT_EQUALS(r->debug_steps_2, 15); // Can only scan here for 120 bytes, 7 left
    TS_ASSERT_EQUALS(r->debug_steps_3, 7); // Newline in last position
#endif


    // Start further ahead
    rc = linescan_find(b+65,cmask,size-65,r);
    TS_ASSERT_EQUALS(63,r->size);
    TS_ASSERT_EQUALS(1,rc);
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 7); // byte search to 72 for alignment 
    TS_ASSERT_EQUALS(r->debug_steps_2, 7); // scan from 72 to 128
    TS_ASSERT_EQUALS(r->debug_steps_3, 8); // Newline in last position
#endif

    // Start close to the end
    rc = linescan_find(b+122,cmask,size-122,r);
    TS_ASSERT_EQUALS(6,r->size);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(2,r->offsets_n);
    {
      auto offsets = std::vector<size_t>{0,5};
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n));
      TS_ASSERT_EQUALS('\n',*(r->buf+r->offsets[1]));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 6); // byte search until 128 
    TS_ASSERT_EQUALS(r->debug_steps_2, 0);  
    TS_ASSERT_EQUALS(r->debug_steps_3, 0);  
#endif


  }

  void test_linescan_find_2(){
    // Start close to the end + additional search hits in step 1
    b[size-1] = '\n';
    b[123] = 'd';
    b[124] = 'd';
    int rc = linescan_find(b+122,cmask,size-122,r);
    TS_ASSERT_EQUALS(6,r->size);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(4,r->offsets_n);
    {
      auto offsets = std::vector<size_t>{0,1,2,5};
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets, r->offsets + r->offsets_n));
      TS_ASSERT_EQUALS('\n',*(r->buf+r->offsets[3]));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 6); // byte search until 128 
    TS_ASSERT_EQUALS(r->debug_steps_2, 0);  
    TS_ASSERT_EQUALS(r->debug_steps_3, 0);  
#endif
  }

  void test_linescan_find_3(){
    // Additional search hits in step 3
    b[size-1] = '\n';
    b[size-3] = 'd';
    b[size-5] = 'd';

    int rc = linescan_find(b+65,cmask,size-65,r);
    TS_ASSERT_EQUALS(63,r->size);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(6,r->offsets_n);
    {
      auto offsets = std::vector<size_t>{0,16,42,58,60,62};
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n));
      TS_ASSERT_EQUALS('\n',*(r->buf+r->offsets[5]));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 7); // byte search to 72 for alignment 
    TS_ASSERT_EQUALS(r->debug_steps_2, 7); // scan from 72 to 128
    TS_ASSERT_EQUALS(r->debug_steps_3, 8); // Newline in last position
#endif
  }
  
  void test_linescan_rfind(){
    b[0] = '\n';
    int rc = linescan_rfind(b,cmask,size,r);
    std::vector<size_t> offsets{0,3,29,55,81,107,127};
    std::reverse(offsets.begin(), offsets.end());
    std::vector<char> offset_chars{'x','d','d','d','d','d','\n'};

    // Normal operations
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(size,r->size);
    TS_ASSERT_EQUALS(b,r->buf);
    TS_ASSERT_EQUALS(7,r->offsets_n);
    TS_ASSERT_EQUALS(size,r->offsets_size);
    for(size_t i=0;i<r->offsets_n;i++){
      TS_ASSERT_EQUALS(offset_chars[i],r->buf[r->offsets[i]]);
    }
    TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n));
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 0); // Aligned, no steps necessary
    TS_ASSERT_EQUALS(r->debug_steps_2, 16); // 16 * 8 = 128
    TS_ASSERT_EQUALS(r->debug_steps_3, 0); // Newline in last position
#endif
    
    // Start further to the left
    rc = linescan_rfind(b,cmask,size-1,r);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(r->size,127);

#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 7); // Search until aligned 
    TS_ASSERT_EQUALS(r->debug_steps_2, 15); // Scan for remaining 120 bytes 
    TS_ASSERT_EQUALS(r->debug_steps_3, 0); 
#endif
    // Start even further to the left
    rc = linescan_rfind(b,cmask,size-123,r);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(r->size,5);
    {
      auto offsets = std::vector<size_t>{4,3,0};
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets, r->offsets + r->offsets_n));
      TS_ASSERT_EQUALS('d',*(r->buf+r->offsets[1]));
      TS_ASSERT_EQUALS('\n',*(r->buf+r->offsets[2]));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 5); // Finish search before alignment
    TS_ASSERT_EQUALS(r->debug_steps_2, 0); 
    TS_ASSERT_EQUALS(r->debug_steps_3, 0);
#endif

    // Start a bit to the left, don't search until newline
    rc = linescan_rfind(b+1,cmask,size-2,r);
    TS_ASSERT_EQUALS(0,rc); // No newline found
    TS_ASSERT_EQUALS(r->size,126);
    TS_ASSERT_EQUALS('w',*(r->buf+r->size-1));
    {
      auto offsets = std::vector<size_t>{125,106,80,54,28,2};
      auto offset_chars = std::vector<char>{'w','d','d','d','d','d'};

      for(size_t i=0;i<r->offsets_n;i++){
	TS_ASSERT_EQUALS(offset_chars[i],*(r->buf+r->offsets[i]));
      }
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 7); // search 7 bytes until alignment
    TS_ASSERT_EQUALS(r->debug_steps_2, 14); // search 112 bytes, leaves 7
    TS_ASSERT_EQUALS(r->debug_steps_3, 7); // search last 7 bytes
#endif
  }

  void test_linescan_rfind_2(){
    // Newline in step 2
    b[size-31] = '\n';
    int rc = linescan_rfind(b,cmask,size,r);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(31,r->size);
    TS_ASSERT_EQUALS(3,r->offsets_n);
    std::vector<size_t> offsets{127,107,97};
    TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets,r->offsets + r->offsets_n));
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 0); // Aligned, no steps necessary
    TS_ASSERT_EQUALS(r->debug_steps_2, 4); // 4 * 8 = 32
    TS_ASSERT_EQUALS(r->debug_steps_3, 0); // Newline found in step 2
#endif
  }

  void test_linescan_rfind_3(){
    // Newline in step 3
    b[4] = '\n';
    int rc = linescan_rfind(b+1,cmask,size-2,r);
    TS_ASSERT_EQUALS(1,rc);
    TS_ASSERT_EQUALS(r->size,123);
    {
      auto offsets = std::vector<size_t>{125,106,80,54,28,3};
      auto offset_chars = std::vector<char>{'w','d','d','d','d','\n'};

      for(size_t i=0;i<r->offsets_n;i++){
	TS_ASSERT_EQUALS(offset_chars[i],*(r->buf+r->offsets[i]));
      }
      TS_ASSERT_EQUALS(offsets,std::vector<size_t>(r->offsets, r->offsets + r->offsets_n));
    }
#ifdef LINESCAN_DEBUG
    TS_ASSERT_EQUALS(r->debug_steps_1, 7); // search 7 bytes until alignment
    TS_ASSERT_EQUALS(r->debug_steps_2, 14); // search 112 bytes, leaves 7
    TS_ASSERT_EQUALS(r->debug_steps_3, 4); // search last 7 bytes
#endif
  }
  
};
