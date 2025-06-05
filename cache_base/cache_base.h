// ECE 430.322: Computer Organization
// Lab 4: Memory System Simulation

#ifndef __CACHE_BASE_H__
#define __CACHE_BASE_H__

#include <cstdint>
#include <string>
#include <list>

typedef enum request_type_enum {
  READ = 0,
  WRITE = 1,
  INST_FETCH = 2,
} request_type;

using addr_t = uint64_t;

///////////////////////////////////////////////////////////////////
class cache_entry_c
{
public:
  cache_entry_c() {} ;
  bool   m_valid;    // valid bit for the cacheline
  bool   m_dirty;    // dirty bit 
  addr_t m_tag;      // tag for the line
  friend class cache_base_c;
};

///////////////////////////////////////////////////////////////////
class cache_set_c 
{
public:
    cache_set_c(int assoc);
    ~cache_set_c();

    cache_entry_c *m_entry;  // array of cache entries. 
    int m_assoc;             // number of cache blocks in a cache set

  ///////////////////////////////////////////////////////////////////
  // TODO: Maintain the LRU stack for this set
    std::list<addr_t> LRU_queue; // queue of set indexes ** FRONT IS MOST RECENT **
    bool full();
    void LRU_update(addr_t idx);
    bool need_to_evict();
    addr_t evict(); // evict and return evicted address
    addr_t find(addr_t tag); // returns idx if tag matches and valid, if no hit return -1, evict and replace if not hit
  ///////////////////////////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////
class cache_base_c 
{
public:
  cache_base_c();
  cache_base_c(std::string name, int num_set, int assoc, int line_size);
  ~cache_base_c();

  bool access(addr_t address, int access_type, bool is_fill);
  bool need_to_evict(addr_t address, int access_type, bool is_fill);
  void print_stats();
  void dump_tag_store(bool is_file);  // false: dump to stdout, true: dump to a file

private:
  std::string m_name;     // cache name
  int m_num_sets;         // number of sets
  int m_line_size;        // cache line size

  cache_set_c **m_set;    // cache data structure, accessed by m_set[set_num]->m_entry[entry_num].m_valid

  // cache statistics
  int m_num_accesses; 
  int m_num_hits; 
  int m_num_misses; 
  int m_num_writes;
  int m_num_writebacks;
};

#endif // !__CACHE_BASE_H__ 
