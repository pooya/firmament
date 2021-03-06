// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
// Copyright (c) 2015 Ionel Gog <ionel.gog@cl.cam.ac.uk>
//
// Coordinated co-location scheduling cost model.

#ifndef FIRMAMENT_SCHEDULING_COCO_COST_MODEL_H
#define FIRMAMENT_SCHEDULING_COCO_COST_MODEL_H

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/common.h"
#include "base/types.h"
#include "misc/utils.h"
#include "scheduling/common.h"
#include "scheduling/knowledge_base.h"
#include "scheduling/flow/cost_model_interface.h"

namespace firmament {

typedef struct CostVector {
  // record number of dimensions here
  const static uint16_t dimensions_ = 8;
  // Data follows
  uint32_t priority_;
  uint32_t cpu_cores_;
  uint32_t ram_gb_;
  uint32_t network_bw_;
  uint32_t disk_bw_;
  uint32_t machine_type_score_;
  uint32_t interference_score_;
  uint32_t locality_score_;
} CostVector_t;

class CocoCostModel : public CostModelInterface {
 public:
  CocoCostModel(shared_ptr<ResourceMap_t> resource_map,
                const ResourceTopologyNodeDescriptor& resource_topology,
                shared_ptr<TaskMap_t> task_map,
                unordered_set<ResourceID_t,
                  boost::hash<boost::uuids::uuid>>* leaf_res_ids,
                KnowledgeBase* kb);
  // Interference score
  uint64_t ComputeInterferenceScore(ResourceID_t res_id);
  // Costs pertaining to leaving tasks unscheduled
  Cost_t TaskToUnscheduledAggCost(TaskID_t task_id);
  Cost_t UnscheduledAggToSinkCost(JobID_t job_id);
  // Per-task costs (into the resource topology)
  Cost_t TaskToResourceNodeCost(TaskID_t task_id,
                                ResourceID_t resource_id);
  // Costs within the resource topology
  Cost_t ResourceNodeToResourceNodeCost(ResourceID_t source,
                                        ResourceID_t destination);
  Cost_t LeafResourceNodeToSinkCost(ResourceID_t resource_id);
  // Costs pertaining to preemption (i.e. already running tasks)
  Cost_t TaskContinuationCost(TaskID_t task_id);
  Cost_t TaskPreemptionCost(TaskID_t task_id);
  // Costs to equivalence class aggregators
  Cost_t TaskToEquivClassAggregator(TaskID_t task_id, EquivClass_t tec);
  Cost_t EquivClassToResourceNode(EquivClass_t tec, ResourceID_t res_id);
  Cost_t EquivClassToEquivClass(EquivClass_t tec1, EquivClass_t tec2);
  int64_t FlattenCostVector(CostVector_t cv);
  // Get the type of equiv class.
  vector<EquivClass_t>* GetTaskEquivClasses(TaskID_t task_id);
  vector<EquivClass_t>* GetResourceEquivClasses(ResourceID_t res_id);
  vector<ResourceID_t>* GetOutgoingEquivClassPrefArcs(EquivClass_t tec);
  vector<TaskID_t>* GetIncomingEquivClassPrefArcs(EquivClass_t tec);
  uint64_t GetInterferenceScoreForTask(TaskID_t task_id);
  vector<ResourceID_t>* GetTaskPreferenceArcs(TaskID_t task_id);
  pair<vector<EquivClass_t>*, vector<EquivClass_t>*>
    GetEquivClassToEquivClassesArcs(EquivClass_t tec);
  uint32_t NormalizeCost(uint64_t raw_cost, uint64_t max_cost);
  void AddMachine(ResourceTopologyNodeDescriptor* rtnd_ptr);
  void AddTask(TaskID_t task_id);
  void PrintCostVector(CostVector_t cv);
  void RemoveMachine(ResourceID_t res_id);
  void RemoveTask(TaskID_t task_id);
  FlowGraphNode* GatherStats(FlowGraphNode* accumulator, FlowGraphNode* other);
  FlowGraphNode* UpdateStats(FlowGraphNode* accumulator, FlowGraphNode* other);

 private:
  // Helper method to get TD for a task ID
  const TaskDescriptor& GetTask(TaskID_t task_id);
  // Cost to cluster aggregator EC
  Cost_t TaskToClusterAggCost(TaskID_t task_id);
  // Fixed value for OMEGA, the normalization ceiling for each dimension's cost
  // value
  const uint64_t omega_ = 1000;
  const Cost_t WAIT_TIME_MULTIPLIER = 1;
  const uint64_t MAX_PRIORITY_VALUE = 10;

  // Lookup maps for various resources from the scheduler.
  shared_ptr<ResourceMap_t> resource_map_;
  const ResourceTopologyNodeDescriptor& resource_topology_;
  shared_ptr<TaskMap_t> task_map_;
  unordered_set<ResourceID_t, boost::hash<boost::uuids::uuid>>* leaf_res_ids_;
  // A knowledge base instance that we will refer to for job runtime statistics.
  KnowledgeBase* knowledge_base_;

  // Mapping between task equiv classes and connected tasks.
  unordered_map<EquivClass_t, set<TaskID_t> > task_ec_to_set_task_id_;
  // Track equivalence class aggregators present
  unordered_set<EquivClass_t> task_aggs_;
  unordered_set<EquivClass_t> machine_aggs_;

  // Largest cost seen so far, plus one
  uint64_t infinity_;
  // Vector to track the maximum capacity values in each dimension
  // present in the cluster (N.B.: these can execeed OMEGA).
  CostVector_t max_capacity_;
};

}  // namespace firmament

#endif  // FIRMAMENT_SCHEDULING_COCO_COST_MODEL_H
