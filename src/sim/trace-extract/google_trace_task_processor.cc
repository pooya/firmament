// Copyright (c) 2015 Ionel Gog <ionel.gog@cl.cam.ac.uk>
// Google resource utilization trace processor.

#include <cstdio>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

#include "sim/trace-extract/google_trace_task_processor.h"
#include "misc/string_utils.h"
#include "dataset_parser.h"

using namespace std;
using boost::lexical_cast;
using boost::algorithm::is_any_of;
using boost::token_compress_off;

namespace firmament {
namespace sim {

  GoogleTraceTaskProcessor::GoogleTraceTaskProcessor(string& trace_path): trace_path_(trace_path) {
  }

  map<uint64_t, vector<TaskSchedulingEvent*> >& GoogleTraceTaskProcessor::ReadTaskSchedulingEvents() {
    // Store the scheduling events for every timestamp.
    map<uint64_t, vector<TaskSchedulingEvent*> > *scheduling_events =
      new map<uint64_t, vector<TaskSchedulingEvent*> >();
    TaskParser tp(trace_path_);
    while (tp.nextRow()) {
    	const TaskEvent &task = tp.getTask();

    	switch (task.event_type) {
    	case JobTaskEventTypes::SCHEDULE:
    	case JobTaskEventTypes::EVICT:
    	case JobTaskEventTypes::FAIL:
    	case JobTaskEventTypes::FINISH:
    	case JobTaskEventTypes::KILL:
    	case JobTaskEventTypes::LOST:
    	{
    		int64_t timestamp = task.timestamp;
    		if (scheduling_events->find(timestamp) == scheduling_events->end()) {
					vector<TaskSchedulingEvent*> events_during_timestamp;
					scheduling_events->insert(
							pair<uint64_t, vector<TaskSchedulingEvent*>>(timestamp,
																											events_during_timestamp));
				}
				TaskSchedulingEvent* event = new TaskSchedulingEvent();
				event->job_id = task.job_id;
				event->task_index = task.task_index;
				event->event_type = task.event_type;
				map<uint64_t, vector<TaskSchedulingEvent*> >::iterator it =
					scheduling_events->find(timestamp);
				it->second.push_back(event);
    		break;
    	}
    	case JobTaskEventTypes::SUBMIT:
    	case JobTaskEventTypes::UPDATE_PENDING:
    	case JobTaskEventTypes::UPDATE_RUNNING:
    		// ignore
    		break;
    	}
    }
    return *scheduling_events;
  }

  TaskResourceUsage* GoogleTraceTaskProcessor::BuildTaskResourceUsage(vector<string>& line_cols) {
    TaskResourceUsage* task_resource_usage = new TaskResourceUsage();
    // Set resource value to -1 if not present. We can then later not take it into account when
    // we compute the statistics.
    for (uint32_t index = 5; index < 17; index++) {
      if (!line_cols[index].compare("")) {
        line_cols[index] = "-1";
      }
    }
    task_resource_usage->mean_cpu_usage = lexical_cast<double>(line_cols[5]);
    task_resource_usage->canonical_mem_usage = lexical_cast<double>(line_cols[6]);
    task_resource_usage->assigned_mem_usage = lexical_cast<double>(line_cols[7]);
    task_resource_usage->unmapped_page_cache = lexical_cast<double>(line_cols[8]);
    task_resource_usage->total_page_cache = lexical_cast<double>(line_cols[9]);
    task_resource_usage->max_mem_usage = lexical_cast<double>(line_cols[10]);
    task_resource_usage->mean_disk_io_time = lexical_cast<double>(line_cols[11]);
    task_resource_usage->mean_local_disk_used = lexical_cast<double>(line_cols[12]);
    task_resource_usage->max_cpu_usage = lexical_cast<double>(line_cols[13]);
    task_resource_usage->max_disk_io_time = lexical_cast<double>(line_cols[14]);
    task_resource_usage->cpi = lexical_cast<double>(line_cols[15]);
    task_resource_usage->mai = lexical_cast<double>(line_cols[16]);
    return task_resource_usage;
  }

  TaskResourceUsage* GoogleTraceTaskProcessor::MinTaskUsage(
      vector<TaskResourceUsage*>& resource_usage) {
    TaskResourceUsage* task_resource_min = new TaskResourceUsage();
    task_resource_min->mean_cpu_usage = numeric_limits<double>::max();
    task_resource_min->canonical_mem_usage = numeric_limits<double>::max();
    task_resource_min->assigned_mem_usage = numeric_limits<double>::max();
    task_resource_min->unmapped_page_cache = numeric_limits<double>::max();
    task_resource_min->total_page_cache = numeric_limits<double>::max();
    task_resource_min->max_mem_usage = numeric_limits<double>::max();
    task_resource_min->mean_disk_io_time = numeric_limits<double>::max();
    task_resource_min->mean_local_disk_used = numeric_limits<double>::max();
    task_resource_min->max_cpu_usage = numeric_limits<double>::max();
    task_resource_min->max_disk_io_time = numeric_limits<double>::max();
    task_resource_min->cpi = numeric_limits<double>::max();
    task_resource_min->mai = numeric_limits<double>::max();
    for (vector<TaskResourceUsage*>::iterator it = resource_usage.begin();
         it != resource_usage.end(); ++it) {
      if ((*it)->mean_cpu_usage >= 0.0) {
        task_resource_min->mean_cpu_usage =
          min(task_resource_min->mean_cpu_usage, (*it)->mean_cpu_usage);
      }
      if ((*it)->canonical_mem_usage >= 0.0) {
        task_resource_min->canonical_mem_usage =
          min(task_resource_min->canonical_mem_usage, (*it)->canonical_mem_usage);
      }
      if ((*it)->assigned_mem_usage >= 0.0) {
        task_resource_min->assigned_mem_usage =
          min(task_resource_min->assigned_mem_usage, (*it)->assigned_mem_usage);
      }
      if ((*it)->unmapped_page_cache >= 0.0) {
        task_resource_min->unmapped_page_cache =
          min(task_resource_min->unmapped_page_cache, (*it)->unmapped_page_cache);
      }
      if ((*it)->total_page_cache >= 0.0) {
        task_resource_min->total_page_cache =
          min(task_resource_min->total_page_cache, (*it)->total_page_cache);
      }
      if ((*it)->max_mem_usage >= 0.0) {
        task_resource_min->max_mem_usage =
          min(task_resource_min->max_mem_usage, (*it)->max_mem_usage);
      }
      if ((*it)->mean_disk_io_time >= 0.0) {
        task_resource_min->mean_disk_io_time =
          min(task_resource_min->mean_disk_io_time, (*it)->mean_disk_io_time);
      }
      if ((*it)->mean_local_disk_used >= 0.0) {
        task_resource_min->mean_local_disk_used =
          min(task_resource_min->mean_local_disk_used, (*it)->mean_local_disk_used);
      }
      if ((*it)->max_cpu_usage >= 0.0) {
        task_resource_min->max_cpu_usage =
          min(task_resource_min->max_cpu_usage, (*it)->max_cpu_usage);
      }
      if ((*it)->max_disk_io_time >= 0.0) {
        task_resource_min->max_disk_io_time =
          min(task_resource_min->max_disk_io_time, (*it)->max_disk_io_time);
      }
      if ((*it)->cpi >= 0.0) {
        task_resource_min->cpi = min(task_resource_min->cpi, (*it)->cpi);
      }
      if ((*it)->mai >= 0.0) {
        task_resource_min->mai = min(task_resource_min->mai, (*it)->mai);
      }
    }
    if (fabs(task_resource_min->mean_cpu_usage - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->mean_cpu_usage = -1.0;
    }
    if (fabs(task_resource_min->canonical_mem_usage - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->canonical_mem_usage = -1.0;
    }
    if (fabs(task_resource_min->assigned_mem_usage - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->assigned_mem_usage = -1.0;
    }
    if (fabs(task_resource_min->unmapped_page_cache - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->unmapped_page_cache = -1.0;
    }
    if (fabs(task_resource_min->total_page_cache - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->total_page_cache = -1.0;
    }
    if (fabs(task_resource_min->max_mem_usage - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->max_mem_usage = -1.0;
    }
    if (fabs(task_resource_min->mean_disk_io_time - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->mean_disk_io_time = -1.0;
    }
    if (fabs(task_resource_min->mean_local_disk_used - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->mean_local_disk_used = -1.0;
    }
    if (fabs(task_resource_min->max_cpu_usage - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->max_cpu_usage = -1.0;
    }
    if (fabs(task_resource_min->max_disk_io_time - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->max_disk_io_time = -1.0;
    }
    if (fabs(task_resource_min->cpi - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->cpi = -1.0;
    }
    if (fabs(task_resource_min->mai - numeric_limits<double>::max()) < 0.00001) {
      task_resource_min->mai = -1.0;
    }
    return task_resource_min;
  }

  TaskResourceUsage* GoogleTraceTaskProcessor::MaxTaskUsage(
      vector<TaskResourceUsage*>& resource_usage) {
    TaskResourceUsage* task_resource_max = new TaskResourceUsage();
    task_resource_max->mean_cpu_usage = -1.0;
    task_resource_max->canonical_mem_usage = -1.0;
    task_resource_max->assigned_mem_usage = -1.0;
    task_resource_max->unmapped_page_cache = -1.0;
    task_resource_max->total_page_cache = -1.0;
    task_resource_max->max_mem_usage = -1.0;
    task_resource_max->mean_disk_io_time = -1.0;
    task_resource_max->mean_local_disk_used = -1.0;
    task_resource_max->max_cpu_usage = -1.0;
    task_resource_max->max_disk_io_time = -1.0;
    task_resource_max->cpi = -1.0;
    task_resource_max->mai = -1.0;
    for (vector<TaskResourceUsage*>::iterator it = resource_usage.begin();
         it != resource_usage.end(); ++it) {
      task_resource_max->mean_cpu_usage =
        max(task_resource_max->mean_cpu_usage, (*it)->mean_cpu_usage);
      task_resource_max->canonical_mem_usage =
        max(task_resource_max->canonical_mem_usage, (*it)->canonical_mem_usage);
      task_resource_max->assigned_mem_usage =
        max(task_resource_max->assigned_mem_usage, (*it)->assigned_mem_usage);
      task_resource_max->unmapped_page_cache =
        max(task_resource_max->unmapped_page_cache, (*it)->unmapped_page_cache);
      task_resource_max->total_page_cache =
        max(task_resource_max->total_page_cache, (*it)->total_page_cache);
      task_resource_max->max_mem_usage =
        max(task_resource_max->max_mem_usage, (*it)->max_mem_usage);
      task_resource_max->mean_disk_io_time =
        max(task_resource_max->mean_disk_io_time, (*it)->mean_disk_io_time);
      task_resource_max->mean_local_disk_used =
        max(task_resource_max->mean_local_disk_used, (*it)->mean_local_disk_used);
      task_resource_max->max_cpu_usage =
        max(task_resource_max->max_cpu_usage, (*it)->max_cpu_usage);
      task_resource_max->max_disk_io_time =
        max(task_resource_max->max_disk_io_time, (*it)->max_disk_io_time);
      task_resource_max->cpi = max(task_resource_max->cpi, (*it)->cpi);
      task_resource_max->mai = max(task_resource_max->mai, (*it)->mai);
    }
    return task_resource_max;
  }

  TaskResourceUsage* GoogleTraceTaskProcessor::AvgTaskUsage(
      vector<TaskResourceUsage*>& resource_usage) {
    TaskResourceUsage* task_resource_avg = new TaskResourceUsage();
    uint64_t num_mean_cpu_usage = 0;
    uint64_t num_canonical_mem_usage = 0;
    uint64_t num_assigned_mem_usage = 0;
    uint64_t num_unmapped_page_cache = 0;
    uint64_t num_total_page_cache = 0;
    uint64_t num_mean_disk_io_time = 0;
    uint64_t num_mean_local_disk_used = 0;
    uint64_t num_cpi = 0;
    uint64_t num_mai = 0;
    for (vector<TaskResourceUsage*>::iterator it = resource_usage.begin();
         it != resource_usage.end(); ++it) {
      // Sum the resources.
      if ((*it)->mean_cpu_usage >= 0.0) {
        task_resource_avg->mean_cpu_usage += (*it)->mean_cpu_usage;
        num_mean_cpu_usage++;
      }
      if ((*it)->canonical_mem_usage >= 0.0) {
        task_resource_avg->canonical_mem_usage += (*it)->canonical_mem_usage;
        num_canonical_mem_usage++;
      }
      if ((*it)->assigned_mem_usage >= 0.0) {
        task_resource_avg->assigned_mem_usage += (*it)->assigned_mem_usage;
        num_assigned_mem_usage++;
      }
      if ((*it)->unmapped_page_cache >= 0.0) {
        task_resource_avg->unmapped_page_cache += (*it)->unmapped_page_cache;
        num_unmapped_page_cache++;
      }
      if ((*it)->total_page_cache >= 0.0) {
        task_resource_avg->total_page_cache += (*it)->total_page_cache;
        num_total_page_cache++;
      }
      if ((*it)->mean_disk_io_time >= 0.0) {
        task_resource_avg->mean_disk_io_time += (*it)->mean_disk_io_time;
        num_mean_disk_io_time++;
      }
      if ((*it)->mean_local_disk_used >= 0.0) {
        task_resource_avg->mean_local_disk_used += (*it)->mean_local_disk_used;
        num_mean_local_disk_used++;
      }
      if ((*it)->cpi >= 0.0) {
        task_resource_avg->cpi += (*it)->cpi;
        num_cpi++;
      }
      if ((*it)->mai >= 0.0) {
        task_resource_avg->mai += (*it)->mai;
        num_mai++;
      }
    }
    if (num_mean_cpu_usage > 0) {
      task_resource_avg->mean_cpu_usage /= num_mean_cpu_usage;
    } else {
      task_resource_avg->mean_cpu_usage = -1.0;
    }
    if (num_canonical_mem_usage > 0) {
      task_resource_avg->canonical_mem_usage /= num_canonical_mem_usage;
    } else {
      task_resource_avg->canonical_mem_usage = -1.0;
    }
    if (num_assigned_mem_usage > 0) {
      task_resource_avg->assigned_mem_usage /= num_assigned_mem_usage;
    } else {
      task_resource_avg->assigned_mem_usage = -1.0;
    }
    if (num_unmapped_page_cache > 0) {
      task_resource_avg->unmapped_page_cache /= num_unmapped_page_cache;
    } else {
      task_resource_avg->unmapped_page_cache = -1.0;
    }
    if (num_total_page_cache > 0) {
      task_resource_avg->total_page_cache /= num_total_page_cache;
    } else {
      task_resource_avg->total_page_cache = -1.0;
    }
    if (num_mean_disk_io_time > 0) {
      task_resource_avg->mean_disk_io_time /= num_mean_disk_io_time;
    } else {
      task_resource_avg->mean_disk_io_time = -1.0;
    }
    if (num_mean_local_disk_used > 0) {
      task_resource_avg->mean_local_disk_used /= num_mean_local_disk_used;
    } else {
      task_resource_avg->mean_local_disk_used = -1.0;
    }
    if (num_cpi > 0) {
      task_resource_avg->cpi /= num_cpi;
    } else {
      task_resource_avg->cpi = -1.0;
    }
    if (num_mai > 0) {
      task_resource_avg->mai /= num_mai;
    } else {
      task_resource_avg->mai = -1.0;
    }
    return task_resource_avg;
  }

  TaskResourceUsage* GoogleTraceTaskProcessor::StandardDevTaskUsage(
      vector<TaskResourceUsage*>& resource_usage) {
    TaskResourceUsage* task_resource_avg = AvgTaskUsage(resource_usage);
    TaskResourceUsage* task_resource_sd = new TaskResourceUsage();
    uint64_t num_mean_cpu_usage = 0;
    uint64_t num_canonical_mem_usage = 0;
    uint64_t num_assigned_mem_usage = 0;
    uint64_t num_unmapped_page_cache = 0;
    uint64_t num_total_page_cache = 0;
    uint64_t num_mean_disk_io_time = 0;
    uint64_t num_mean_local_disk_used = 0;
    uint64_t num_cpi = 0;
    uint64_t num_mai = 0;
    for (vector<TaskResourceUsage*>::iterator it = resource_usage.begin();
         it != resource_usage.end(); ++it) {
      if ((*it)->mean_cpu_usage >= 0.0) {
        task_resource_sd->mean_cpu_usage +=
          pow((*it)->mean_cpu_usage - task_resource_avg->mean_cpu_usage, 2);
        num_mean_cpu_usage++;
      }
      if ((*it)->canonical_mem_usage >= 0.0) {
        task_resource_sd->canonical_mem_usage +=
          pow((*it)->canonical_mem_usage - task_resource_avg->canonical_mem_usage, 2);
        num_canonical_mem_usage++;
      }
      if ((*it)->assigned_mem_usage >= 0.0) {
        task_resource_sd->assigned_mem_usage +=
          pow((*it)->assigned_mem_usage - task_resource_avg->assigned_mem_usage, 2);
        num_assigned_mem_usage++;
      }
      if ((*it)->unmapped_page_cache >= 0.0) {
        task_resource_sd->unmapped_page_cache +=
          pow((*it)->unmapped_page_cache - task_resource_avg->unmapped_page_cache, 2);
        num_unmapped_page_cache++;
      }
      if ((*it)->total_page_cache >= 0.0) {
        task_resource_sd->total_page_cache +=
          pow((*it)->total_page_cache - task_resource_avg->total_page_cache, 2);
        num_total_page_cache++;
      }
      if ((*it)->mean_disk_io_time >= 0.0) {
        task_resource_sd->mean_disk_io_time +=
          pow((*it)->mean_disk_io_time - task_resource_avg->mean_disk_io_time, 2);
        num_mean_disk_io_time++;
      }
      if ((*it)->mean_local_disk_used >= 0.0) {
        task_resource_sd->mean_local_disk_used +=
          pow((*it)->mean_local_disk_used - task_resource_avg->mean_local_disk_used, 2);
        num_mean_local_disk_used++;
      }
      if ((*it)->cpi >= 0.0) {
        task_resource_sd->cpi += pow((*it)->cpi - task_resource_avg->cpi, 2);
        num_cpi++;
      }
      if ((*it)->mai >= 0.0) {
        task_resource_sd->mai += pow((*it)->mai - task_resource_avg->mai, 2);
        num_mai++;
      }
    }
    if (num_mean_cpu_usage > 0) {
      task_resource_sd->mean_cpu_usage =
        sqrt(task_resource_sd->mean_cpu_usage / num_mean_cpu_usage);
    } else {
      task_resource_sd->mean_cpu_usage = -1.0;
    }
    if (num_canonical_mem_usage > 0) {
      task_resource_sd->canonical_mem_usage =
        sqrt(task_resource_sd->canonical_mem_usage / num_canonical_mem_usage);
    } else {
      task_resource_sd->canonical_mem_usage = -1.0;
    }
    if (num_assigned_mem_usage > 0) {
      task_resource_sd->assigned_mem_usage =
        sqrt(task_resource_sd->assigned_mem_usage / num_assigned_mem_usage);
    } else {
      task_resource_sd->assigned_mem_usage = -1.0;
    }
    if (num_unmapped_page_cache > 0) {
      task_resource_sd->unmapped_page_cache =
        sqrt(task_resource_sd->unmapped_page_cache / num_unmapped_page_cache);
    } else {
      task_resource_sd->unmapped_page_cache = -1.0;
    }
    if (num_total_page_cache > 0) {
      task_resource_sd->total_page_cache =
        sqrt(task_resource_sd->total_page_cache / num_total_page_cache);
    } else {
      task_resource_sd->total_page_cache = -1.0;
    }
    if (num_mean_disk_io_time > 0) {
      task_resource_sd->mean_disk_io_time =
        sqrt(task_resource_sd->mean_disk_io_time / num_mean_disk_io_time);
    } else {
      task_resource_sd->mean_disk_io_time = -1.0;
    }
    if (num_mean_local_disk_used > 0) {
      task_resource_sd->mean_local_disk_used =
        sqrt(task_resource_sd->mean_local_disk_used / num_mean_local_disk_used);
    } else {
      task_resource_sd->mean_local_disk_used = -1.0;
    }
    if (num_cpi > 0) {
      task_resource_sd->cpi = sqrt(task_resource_sd->cpi / num_cpi);
    } else {
      task_resource_sd->cpi = -1.0;
    }
    if (num_mai > 0) {
      task_resource_sd->mai = sqrt(task_resource_sd->mai / num_mai);
    } else {
      task_resource_sd->mai = -1.0;
    }
    return task_resource_sd;
  }

  void GoogleTraceTaskProcessor::PrintStats(FILE* usage_stat_file, uint64_t job_id,
                                            uint64_t task_index,
                                            vector<TaskResourceUsage*>& task_resource) {
    TaskResourceUsage* avg_task_usage = AvgTaskUsage(task_resource);
    TaskResourceUsage* min_task_usage = MinTaskUsage(task_resource);
    TaskResourceUsage* max_task_usage = MaxTaskUsage(task_resource);
    TaskResourceUsage* sd_task_usage = StandardDevTaskUsage(task_resource);
    fprintf(usage_stat_file,
            "%" PRId64 " %" PRId64 " %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
            job_id, task_index,
            min_task_usage->mean_cpu_usage, max_task_usage->mean_cpu_usage, avg_task_usage->mean_cpu_usage, sd_task_usage->mean_cpu_usage,
            min_task_usage->canonical_mem_usage, max_task_usage->canonical_mem_usage, avg_task_usage->canonical_mem_usage, sd_task_usage->canonical_mem_usage,
            min_task_usage->assigned_mem_usage, max_task_usage->assigned_mem_usage, avg_task_usage->assigned_mem_usage, sd_task_usage->assigned_mem_usage,
            min_task_usage->unmapped_page_cache, max_task_usage->unmapped_page_cache, avg_task_usage->unmapped_page_cache, sd_task_usage->unmapped_page_cache,
            min_task_usage->total_page_cache, max_task_usage->total_page_cache, avg_task_usage->total_page_cache, sd_task_usage->total_page_cache,
            min_task_usage->mean_disk_io_time, max_task_usage->mean_disk_io_time, avg_task_usage->mean_disk_io_time, sd_task_usage->mean_disk_io_time,
            min_task_usage->mean_local_disk_used, max_task_usage->mean_local_disk_used, avg_task_usage->mean_local_disk_used, sd_task_usage->mean_local_disk_used,
            min_task_usage->cpi, max_task_usage->cpi, avg_task_usage->cpi, sd_task_usage->cpi,
            min_task_usage->mai, max_task_usage->mai, avg_task_usage->mai, sd_task_usage->mai);
    delete avg_task_usage;
    delete min_task_usage;
    delete max_task_usage;
    delete sd_task_usage;
  }

  void GoogleTraceTaskProcessor::AggregateTaskUsage() {
    map<uint64_t, vector<TaskSchedulingEvent*> >& scheduling_events = ReadTaskSchedulingEvents();
    // Map job id to map of task id to vector of resource usage.
    map<uint64_t, map<uint64_t, vector<TaskResourceUsage*> > > task_usage;
    char line[200];
    vector<string> line_cols;
    FILE* usage_file = NULL;
    FILE* usage_stat_file = NULL;
    string usage_file_name;
    spf(&usage_file_name, "%s/task_usage_stat/task_usage_stat.csv", trace_path_.c_str());
    if ((usage_stat_file = fopen(usage_file_name.c_str(), "w")) == NULL) {
      LOG(ERROR) << "Failed to open task_usage_stat file for writing";
    }
    uint64_t last_timestamp = 0;
    for (uint64_t file_num = 0; file_num < 500; file_num++) {
      string file_name;
      spf(&file_name, "%s/task_usage/part-%05ld-of-00500.csv", trace_path_.c_str(), file_num);
      if ((usage_file = fopen(file_name.c_str(), "r")) == NULL) {
        LOG(ERROR) << "Failed to open trace for reading of task resource usage.";
      }
      int64_t num_line = 1;
      while (!feof(usage_file)) {
        if (fscanf(usage_file, "%[^\n]%*[\n]", &line[0]) > 0) {
          boost::split(line_cols, line, is_any_of(","), token_compress_off);
          if (line_cols.size() != 19) {
            LOG(ERROR) << "Unexpected structure of task usage on line " << num_line << ": found "
                       << line_cols.size() << " columns.";
          } else {
            uint64_t start_timestamp = lexical_cast<uint64_t>(line_cols[0]);
            uint64_t job_id = lexical_cast<uint64_t>(line_cols[2]);
            uint64_t task_index = lexical_cast<uint64_t>(line_cols[3]);
            if (last_timestamp < start_timestamp) {
              while (1) {
                map<uint64_t, vector<TaskSchedulingEvent*> >::iterator first_entry =
                  scheduling_events.begin();
                if (first_entry->first <= last_timestamp) {
                  for (vector<TaskSchedulingEvent*>::iterator evt_it = first_entry->second.begin();
                       evt_it != first_entry->second.end(); ++evt_it) {
                    // TODO(ionel): Which events should I handle here?
                    if ((*evt_it)->event_type == JobTaskEventTypes::FINISH) {
                      uint64_t job_id = (*evt_it)->job_id;
                      uint64_t task_index = (*evt_it)->task_index;
                      PrintStats(usage_stat_file, job_id, task_index,
                                 task_usage[job_id][task_index]);
                      // Free task resource usage memory.
                      vector<TaskResourceUsage*>::iterator res_it =
                        task_usage[job_id][task_index].begin();
                      vector<TaskResourceUsage*>::iterator end_it =
                        task_usage[job_id][task_index].end();
                      for (; res_it != end_it; res_it++) {
                        delete *res_it;
                      }
                      task_usage[job_id][task_index].clear();
                      task_usage[job_id].erase(task_index);
                      if (task_usage[job_id].empty()) {
                        task_usage.erase(job_id);
                      }
                    }
                  }
                  // Free task scheduling event memory.
                  vector<TaskSchedulingEvent*>::iterator schd_it = first_entry->second.begin();
                  vector<TaskSchedulingEvent*>::iterator end_it = first_entry->second.end();
                  for (; schd_it != end_it; schd_it++) {
                    delete *schd_it;
                  }
                  first_entry->second.clear();
                  scheduling_events.erase(scheduling_events.begin());
                } else {
                  break;
                }
              }
              last_timestamp = start_timestamp;
            }

            TaskResourceUsage* task_resource_usage = BuildTaskResourceUsage(line_cols);

            if (task_usage.find(job_id) == task_usage.end()) {
              // New job id.
              map<uint64_t, vector<TaskResourceUsage*> > task_to_usage;
              task_usage.insert(
                  pair<uint64_t, map<uint64_t, vector<TaskResourceUsage*> > >(job_id,
                                                                              task_to_usage));
            }
            if (task_usage[job_id].find(task_index) == task_usage[job_id].end()) {
              // New task index.
              vector<TaskResourceUsage*> resource_usage;
              resource_usage.push_back(task_resource_usage);
              task_usage[job_id].insert(
                  pair<uint64_t, vector<TaskResourceUsage*> >(task_index, resource_usage));
            } else {
              task_usage[job_id][task_index].push_back(task_resource_usage);
            }
          }
        }
        num_line++;
      }
    }
    // Write stats for tasks that are still running.
    for (map<uint64_t, map<uint64_t, vector<TaskResourceUsage*> > >::iterator job_it = task_usage.begin();
         job_it != task_usage.end(); job_it++) {
      for (map<uint64_t, vector<TaskResourceUsage*> >::iterator task_it = job_it->second.begin();
           task_it != job_it->second.end(); task_it++) {
        PrintStats(usage_stat_file, job_it->first, task_it->first, task_it->second);
      }
    }
    // TODO(ionel): Free the memory occupied by the jobs running at the end of the trace.
    fclose(usage_stat_file);
  }

  // Returns a mapping job id to logical job name.
  map<uint64_t, string>& GoogleTraceTaskProcessor::ReadLogicalJobsName() {
    map<uint64_t, string> *job_id_to_name = new map<uint64_t, string>();
    JobParser jp(trace_path_);
    while (jp.nextRow()) {
    	const JobEvent &job = jp.getJob();

    	job_id_to_name->insert(pair<uint64_t, string>
    												  (job.job_id, job.logical_job_name));
    }

    return *job_id_to_name;
  }

  void GoogleTraceTaskProcessor::PrintTaskRuntime(FILE* out_events_file,
  		TaskRuntime* task_runtime, string logical_job_name, uint64_t runtime,
			const TaskEvent &task) {
    fprintf(out_events_file, "%" PRId64 " %" PRId32 " %s %" PRId64 " %" PRId64 " %" PRId64 " %" PRId64 " %" PRId64 " %" PRId64 " %lf %lf %lf %d\n",
            task.job_id, task.task_index, logical_job_name.c_str(),
            task_runtime->start_time, task_runtime->total_runtime, runtime, task_runtime->num_runs,
            task.scheduling_class, task.priority, task.cpu_request,
						task.ram_request, task.disk_request, task.machine_constraint);

  }

  void GoogleTraceTaskProcessor::ExpandTaskEvents() {
    map<uint64_t, string> job_id_to_name = ReadLogicalJobsName();
    map<uint64_t, map<uint64_t, TaskRuntime*> > tasks_runtime;
    FILE* out_events_file = NULL;
    string out_file_name;
    spf(&out_file_name, "%s/task_runtime_events/task_runtime_events.csv", trace_path_.c_str());
    if ((out_events_file = fopen(out_file_name.c_str(), "w")) == NULL) {
      LOG(ERROR) << "Failed to open task_runtime_events file for writing";
    }

    TaskParser tp(trace_path_);
    while (tp.nextRow()) {
    	const TaskEvent &task = tp.getTask();
    	uint64_t job_id = task.job_id;

    	if (tasks_runtime.find(job_id) == tasks_runtime.end()) {
    		// New job id.
				map<uint64_t, TaskRuntime*> task_runtime;
				tasks_runtime.insert(pair<uint64_t, map<uint64_t, TaskRuntime*>>
						                  (job_id, task_runtime));
    	}

    	uint64_t timestamp = task.timestamp;
    	uint32_t task_index = task.task_index;
    	switch (task.event_type) {
    	case JobTaskEventTypes::SCHEDULE:
    		if (tasks_runtime[job_id].find(task_index) == tasks_runtime[job_id].end()) {
					// New task.
					TaskRuntime* task_runtime = new TaskRuntime();
					task_runtime->start_time = timestamp;
					task_runtime->last_schedule_time = timestamp;
					tasks_runtime[job_id].insert(pair<uint64_t, TaskRuntime*>(task_index,
																																		task_runtime));
				} else {
					tasks_runtime[job_id][task_index]->last_schedule_time = timestamp;
				}
    		break;
    	case JobTaskEventTypes::EVICT:
    	case JobTaskEventTypes::FAIL:
    	case JobTaskEventTypes::KILL:
    	case JobTaskEventTypes::LOST:
    		if (tasks_runtime[job_id].find(task_index) == tasks_runtime[job_id].end()) {
					// First event for this task.
					TaskRuntime* task_runtime = new TaskRuntime();
					task_runtime->start_time = 0;
					task_runtime->num_runs = 1;
					task_runtime->total_runtime = timestamp;
					tasks_runtime[job_id].insert(pair<uint64_t, TaskRuntime*>(task_index,
																																		task_runtime));
				} else {
					TaskRuntime* task_runtime = tasks_runtime[job_id][task_index];
					task_runtime->num_runs++;
					task_runtime->total_runtime += timestamp - task_runtime->last_schedule_time;
				}
    		break;
    	case JobTaskEventTypes::FINISH:
    	{
    		string logical_job_name = job_id_to_name[job_id];
				if (tasks_runtime[job_id].find(task_index) == tasks_runtime[job_id].end()) {
					// First event for this task.
					TaskRuntime* task_runtime = new TaskRuntime();
					task_runtime->start_time =0;
					task_runtime->num_runs = 1;
					task_runtime->total_runtime = timestamp;
					tasks_runtime[job_id].insert(pair<uint64_t, TaskRuntime*>(task_index,
																																		task_runtime));
					PrintTaskRuntime(out_events_file, task_runtime, logical_job_name,
							             timestamp, task);
				} else {
					TaskRuntime* task_runtime = tasks_runtime[job_id][task_index];
					task_runtime->num_runs++;
					task_runtime->total_runtime += timestamp - task_runtime->last_schedule_time;
					PrintTaskRuntime(out_events_file, task_runtime, logical_job_name,
							             timestamp - task_runtime->last_schedule_time, task);
				}
				break;
    	}
    	case JobTaskEventTypes::SUBMIT:
    	case JobTaskEventTypes::UPDATE_PENDING:
			case JobTaskEventTypes::UPDATE_RUNNING:
				// ignore
				break;
    	}
    }

    // TODO(ionel): Free memory.
    fclose(out_events_file);
  }

  void GoogleTraceTaskProcessor::Run() {
    // TODO(ionel): Implement.
    //    AggregateTaskUsage();
    ExpandTaskEvents();
  }

} // sim
} // firmamment
