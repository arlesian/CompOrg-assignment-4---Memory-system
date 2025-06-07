// ECE 430.322: Computer Organization
// Lab 4: Memory System Simulation

/**
 *
 * This is the base cache structure that maintains and updates the tag store
 * depending on a cache hit or a cache miss. Note that the implementation here
 * will be used throughout Lab 4. 
 */

#include "cache_base.h"

#include <cmath>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>

/**
 * This allocates an "assoc" number of cache entries per a set
 * @param assoc - number of cache entries in a set
 */
cache_set_c::cache_set_c(int assoc) {
  m_entry = new cache_entry_c[assoc];
  m_assoc = assoc;
  LRU_queue.clear();
  for (int i = 0; i < m_assoc; ++i) LRU_queue.push_front(i);
} // need LRU_stack initialization logic

// cache_set_c destructor
cache_set_c::~cache_set_c() {
  delete[] m_entry;
}


// I MADE THIS!!!
addr_t cache_set_c::find(addr_t tag){
  for (int i = 0; i < m_assoc; ++i) {
    if (( m_entry[i].m_tag == tag ) && m_entry[i].m_valid ){
      return i;
    }
  }
  return -1;
}

addr_t cache_set_c::evict(){
  for (int i = 0; i < m_assoc; ++i){
    if (m_entry[i].m_valid == false) return i;
  }

  // if no invalid slots, return the LRU val
  addr_t returnVal = LRU_queue.back();
  // update is done in access
  return returnVal; 
}

void cache_set_c::LRU_update(addr_t idx){
  LRU_queue.remove(idx);
  LRU_queue.push_front(idx);
}

/**
 * This constructor initializes a cache structure based on the cache parameters.
 * @param name - cache name; use any name you want
 * @param num_sets - number of sets in a cache
 * @param assoc - number of cache entries in a set
 * @param line_size - cache block (line) size in bytes
 *
 * @note You do not have to modify this (other than for debugging purposes).
 */
cache_base_c::cache_base_c(std::string name, int num_sets, int assoc, int line_size) {
  m_name = name;
  m_num_sets = num_sets;
  m_line_size = line_size;

  m_set = new cache_set_c *[m_num_sets];

  for (int ii = 0; ii < m_num_sets; ++ii) {
    m_set[ii] = new cache_set_c(assoc);

    // initialize tag/valid/dirty bits
    for (int jj = 0; jj < assoc; ++jj) {
      m_set[ii]->m_entry[jj].m_valid = false; // ii'th set jjth entry
      m_set[ii]->m_entry[jj].m_dirty = false;
      m_set[ii]->m_entry[jj].m_tag   = 0;
    }
  }

  // initialize stats
  m_num_accesses = 0;
  m_num_hits = 0;
  m_num_misses = 0;
  m_num_writes = 0;
  m_num_writebacks = 0;
}



// cache_base_c destructor
cache_base_c::~cache_base_c() {
  for (int ii = 0; ii < m_num_sets; ++ii) { delete m_set[ii]; }
  delete[] m_set;
}

//////////////////
// NOTE TO SELF //
//////////////////
// cache_set has an array of cache_entries(an array of cache_entry* s precisely)
// cache_base has an array of cache_sets(an array of cache_set* s precisely)
// so all accessed by ->


/** 
 * This function looks up in the cache for a memory reference.
 * This needs to update all the necessary meta-data (e.g., tag/valid/dirty) 
 * and the cache statistics, depending on a cache hit or a miss.
 * @param address - memory address 
 * @param access_type - read (0), write (1), or instruction fetch (2)
 * @param is_fill - if the access is for a cache fill
 * @param return "true" on a hit; "false" otherwise.
 */
bool cache_base_c::access(addr_t address, int access_type, bool is_fill) {
  ////////////////////////////////////////////////////////////////////
  // TODO: Write the code to implement this function

  // granulity, line, set bits in addr_t
  addr_t line_G       = m_line_size;
  addr_t set_line_G   = m_num_sets * line_G;

  // address decoded
  addr_t line_address = ( address % line_G );              // decide which byte to access
  addr_t set_address  = ( address % set_line_G ) / line_G; // decide which set to index
  addr_t tag_bits     = address / set_line_G;              // leftover is the tag bits

  // current cache entry info
  cache_set_c* current_set = m_set[set_address];
  
  // returns -1 on miss, set idx on hit
  addr_t set_idx = current_set->find(tag_bits);

  if (set_idx == -1){

    // if (!is_fill){ // for Part II
    m_num_accesses++;
    m_num_misses++;
    if (access_type == request_type::WRITE) m_num_writes++;
    // }

    addr_t evict_idx = current_set->evict(); // NEED TO TAKE CARE OF DIRTY!!!
    cache_entry_c* evict_entry = ( current_set->m_entry + evict_idx );

    // this !is_fill is for Part II
    if (evict_entry->m_dirty && evict_entry->m_valid /*&& !is_fill*/) m_num_writebacks++; 
    // fill is already hit ( when evict, replaced with fill data tag )
    
    evict_entry->m_tag = tag_bits;
    evict_entry->m_valid = true;

    evict_entry->m_dirty = (access_type == request_type::WRITE /* && !is_fill*/ );
    // update variables

    if (access_type != request_type::WB)
      current_set->LRU_update(evict_idx); // dont update LRU for wb

    return false;
    
  } else {

    cache_entry_c* current_entry = ( current_set->m_entry + set_idx );

    // if (!is_fill){ // for Part II
      m_num_accesses++;
      m_num_hits++;
      if (access_type == request_type::WRITE) m_num_writes++;
    // }

    if (access_type == request_type::WRITE) current_entry->m_dirty = true;

    // WE FILED ANOTHER REQ FOR FILL AT THE PROCCESS_FILL so need not count again

    current_set->LRU_update(set_idx); // hit wb is write hit

    return true;

  }

  std::cerr << "not supposed to reach here" << std::endl;
  assert(false);

  return true;

  ////////////////////////////////////////////////////////////////////
}



/**
 * Print statistics (DO NOT CHANGE)
 */
void cache_base_c::print_stats() {
  std::cout << "------------------------------" << "\n";
  std::cout << m_name << " Hit Rate: "          << (double)m_num_hits/m_num_accesses*100 << " % \n";
  std::cout << "------------------------------" << "\n";
  std::cout << "number of accesses: "    << m_num_accesses << "\n";
  std::cout << "number of hits: "        << m_num_hits << "\n";
  std::cout << "number of misses: "      << m_num_misses << "\n";
  std::cout << "number of writes: "      << m_num_writes << "\n";
  std::cout << "number of writebacks: "  << m_num_writebacks << "\n";
}


/**
 * Dump tag store (for debugging) 
 * Modify this if it does not dump from the MRU to LRU positions in your implementation.
 */
void cache_base_c::dump_tag_store(bool is_file) {
  auto write = [&](std::ostream &os) { 
    os << "------------------------------" << "\n";
    os << m_name << " Tag Store\n";
    os << "------------------------------" << "\n";

    for (int ii = 0; ii < m_num_sets; ii++) {
      for (int jj = 0; jj < m_set[0]->m_assoc; jj++) {
        os << "[" << (int)m_set[ii]->m_entry[jj].m_valid << ", ";
        os << (int)m_set[ii]->m_entry[jj].m_dirty << ", ";
        os << std::setw(10) << std::hex << m_set[ii]->m_entry[jj].m_tag << std::dec << "] ";
      }
      os << "\n";
    }
  };

  if (is_file) {
    std::ofstream ofs(m_name + ".dump");
    write(ofs);
  } else {
    write(std::cout);
  }
}
