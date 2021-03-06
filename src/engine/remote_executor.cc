// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Remote executor class.

#include "engine/remote_executor.h"

#include "base/common.h"
#include "misc/protobuf_envelope.h"
#include "misc/map-util.h"
#include "messages/base_message.pb.h"
#include "messages/task_delegation_message.pb.h"
#include "misc/utils.h"


// XXX(malte): hack
DECLARE_string(listen_uri);

namespace firmament {
namespace executor {

RemoteExecutor::RemoteExecutor(
    ResourceID_t resource_id,
    ResourceID_t coordinator_resource_id,
    const string& coordinator_uri,
    ResourceMap_t* res_map,
    MessagingAdapterInterface<BaseMessage>* m_adapter_ptr)
    : managing_coordinator_uri_(coordinator_uri),
      remote_resource_id_(resource_id),
      local_resource_id_(coordinator_resource_id),
      res_map_ptr_(res_map),
      m_adapter_ptr_(m_adapter_ptr) {
}

bool RemoteExecutor::CheckRunningTasksHealth(vector<TaskID_t>* failed_tasks) {
  return true;
}

void RemoteExecutor::HandleTaskCompletion(TaskDescriptor* td,
                                          TaskFinalReport* report) {
  uint64_t end_time = GetCurrentTimestamp();
  td->set_finish_time(end_time);
}

void RemoteExecutor::HandleTaskEviction(TaskDescriptor* td) {
  // TODO(ionel): Implement.
}

void RemoteExecutor::HandleTaskFailure(TaskDescriptor* td) {
  td->set_finish_time(GetCurrentTimestamp());
}

void RemoteExecutor::RunTask(TaskDescriptor* td, bool firmament_binary) {
  MessagingChannelInterface<BaseMessage>* chan = GetChannel();
  SendTaskExecutionMessage(chan, td, firmament_binary);

  td->set_start_time(GetCurrentTimestamp());
}

MessagingChannelInterface<BaseMessage>* RemoteExecutor::GetChannel() {
  ResourceStatus** rs_ptr = FindOrNull(*res_map_ptr_, remote_resource_id_);
  CHECK(rs_ptr) << "Resource " << remote_resource_id_ << " appears to no "
                << "longer exist in the resource map!";
  const string remote_endpoint = (*rs_ptr)->location();
  VLOG(1) << "Remote task spawn on resource " << remote_resource_id_
          << ", endpoint " << remote_endpoint;
  MessagingChannelInterface<BaseMessage>* chan =
      m_adapter_ptr_->GetChannelForEndpoint(remote_endpoint);
  return chan;
}

bool RemoteExecutor::SendTaskExecutionMessage(
    MessagingChannelInterface<BaseMessage>* chan,
    TaskDescriptor* td, bool /*firmament_binary*/) {
  CHECK_NOTNULL(chan);
  // Craft a task execution message
  BaseMessage exec_message;
  TaskDescriptor* msg_td =
      exec_message.mutable_task_delegation_request()->
          mutable_task_descriptor();
  // N.B. copies task descriptor
  msg_td->CopyFrom(*td);
  // Prepare delegation message
  msg_td->set_delegated_from(FLAGS_listen_uri);
  SUBMSG_WRITE(exec_message, task_delegation_request, target_resource_id,
               to_string(remote_resource_id_));
  SUBMSG_WRITE(exec_message, task_delegation_request, delegating_resource_id,
               to_string(local_resource_id_));
  Envelope<BaseMessage> envelope(&exec_message);
  // Send it to the relevant resource's coordinator
  CHECK(chan->SendS(envelope));
  // Receive the response
  //chan->RecvS();
  // XXX(malte): need to check here if the delegation succeeded
  // Mark as delegated
  td->set_state(TaskDescriptor::DELEGATED);
  return true;
}

}  // namespace executor
}  // namespace firmament
