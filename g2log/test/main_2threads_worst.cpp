/* *************************************************
// * Filename: g2log-main_mean.cpp
// * Created: 2011 by Kjell Hedström
// *
// * PUBLIC DOMAIN and Not copy-writed
// * ********************************************* */

// through CMakeLists.txt   #define of GOOGLE_GLOG_PERFORMANCE and G2LOG_PERFORMANCE
#include "performance.h"

#include <thread>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

#if defined(G2LOG_PERFORMANCE)
const std::string title = "G2LOG";
#elif defined(GOOGLE_GLOG_PERFORMANCE)
const std::string title = "GOOGLE__GLOG";
#else
#error G2LOG_PERFORMANCE or GOOGLE_GLOG_PERFORMANCE was not defined
#endif

const std::string  g_prefix_log_name = title + "-performance-2threads-WORST_LOG";
const std::string  g_measurement_dump= "/tmp/" + g_prefix_log_name + "_RESULT.txt";
const std::string  g_measurement_bucket_dump= "/tmp/" + g_prefix_log_name + "_RESULT_buckets.txt";
const std::string g_path = "/tmp/";
using namespace g2_test;


int main(int argc, char** argv)
{
  std::ostringstream oss;
  oss << "\n\n" << title << " performance 2 threads WORST (PEAK) times\n";
  oss << "Each thread running #: " << g_loop << " * " << g_iterations << " iterations of log entries" << std::endl;  // worst mean case is about 10us per log entry
  const size_t xtra_margin = 2;
  oss << "*** It can take som time. Please wait: Approximate maximum wait time is:  " << (long long) (g_iterations * 10 * xtra_margin / 1000000 ) << " seconds" << std::endl;
  writeTextToFile(g_measurement_dump, oss.str(), kAppend);
  oss.str(""); // clear the stream

#if defined(G2LOG_PERFORMANCE)
  LogWorker* logger = new LogWorker(g_prefix_log_name, g_path);
  g2::initializeLogging(logger);
#elif defined(GOOGLE_GLOG_PERFORMANCE)
  google::InitGoogleLogging(argv[0]);
#endif

  std::vector<long long> t1_result;
  std::vector<long long> t2_result;
  t1_result.reserve(g_iterations);
  t2_result.reserve(g_iterations);

  auto start_time = std::chrono::steady_clock::now();
  // running both threads
  std::thread t1(measurePeakDuringLogWrites, "g2log_T1", std::ref(t1_result));
  std::thread t2(measurePeakDuringLogWrites, "g2log_T2", std::ref(t2_result));
  t1.join();
  t2.join();
  auto application_end_time = std::chrono::steady_clock::now();

#if defined(G2LOG_PERFORMANCE)
  delete logger; // will flush anything in the queue to file
#elif defined(GOOGLE_GLOG_PERFORMANCE)
  google::ShutdownGoogleLogging();
#endif

  auto worker_end_time = std::chrono::steady_clock::now();
  auto application_time_us = std::chrono::duration_cast<microsecond>(application_end_time - start_time).count();
  auto total_time_us = std::chrono::duration_cast<microsecond>(worker_end_time - start_time).count();

  oss << "\n" << g_iterations << " log entries took: [" << total_time_us / 1000000 << " s] to write to disk"<< std::endl;
  oss << "[Application(2threads+overhead time for measurement):\t" << application_time_us/1000 << " ms]" << std::endl;
  oss << "[Background thread to finish:\t\t\t\t" << total_time_us/1000 << " ms]" << std::endl;
  oss << "\nAverage time per log entry:" << std::endl;
  oss << "[Application: " << application_time_us/g_iterations << " us]" << std::endl;
  oss << "[Application t1: worst took: " <<  (*std::max_element(t1_result.begin(), t1_result.end())) / 1000 << " ms]" << std::endl;
  oss << "[Application t2: worst took: " <<  (*std::max_element(t2_result.begin(), t2_result.end())) / 1000 << " ms]" << std::endl;
  writeTextToFile(g_measurement_dump,oss.str(), kAppend);
  std::cout << "Result can be found at:" << g_measurement_dump << std::endl;


  // now split the result in buckets of 10ms each so that it's obvious how the peaks go
  std::vector<long long> all_measurements;
  all_measurements.reserve(t1_result.size() + t1_result.size());
  all_measurements.insert(all_measurements.end(), t1_result.begin(), t1_result.end());
  all_measurements.insert(all_measurements.end(), t2_result.begin(), t2_result.end());
  std::sort (all_measurements.begin(), all_measurements.end());
  std::map<size_t, size_t> value_amounts;
  // for(long long& idx : all_measurements) --- didn't work?!
  for(auto iter = all_measurements.begin(); iter != all_measurements.end(); ++iter)
  {
    auto value = (*iter)/1000; // convert to ms
    //auto bucket=floor(value*10+0.5)/10;
    ++value_amounts[value]; // asuming size_t is default 0 when initialized
  }

  oss.str("");
  oss << "Number of values rounted to milliseconds and put to [millisecond bucket] were dumped to file: " << g_measurement_bucket_dump << std::endl;
  oss << "Format:  bucket_of_ms, number_of_values_in_bucket";
  std::cout << oss.str() << std::endl;

  for(auto iter = value_amounts.begin(); iter != value_amounts.end(); ++iter)
  {
    oss << iter->first << "\t, " << iter->second << std::endl;
  }
  writeTextToFile(g_measurement_bucket_dump,oss.str(), kAppend,  false);


  return 0;
}
