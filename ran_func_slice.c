/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "ran_func_slice.h"
#include "../../flexric/test/rnd/fill_rnd_data_slice.h"
#include <assert.h>
#include <stdio.h>
#include "common/ran_context.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include <time.h>



bool read_slice_sm(void* data)
{
  assert(data != NULL);

  slice_ind_data_t* slice = (slice_ind_data_t*)data;
  
  /* Initialize empty DL slice configuration */
  slice->msg.slice_conf.dl.len_slices = 0;
  slice->msg.slice_conf.dl.slices = NULL;

  /* DO NOT SEND NULL STRING */
  slice->msg.slice_conf.dl.len_sched_name = 1;
  slice->msg.slice_conf.dl.sched_name = calloc(1, 1);
  slice->msg.slice_conf.dl.sched_name[0] = '\0';


  /* Initialize empty UL slice configuration */
  slice->msg.slice_conf.ul.len_slices = 0;
  slice->msg.slice_conf.ul.slices = NULL;

  /* DO NOT SEND NULL STRING */
  slice->msg.slice_conf.ul.len_sched_name = 1;
  slice->msg.slice_conf.ul.sched_name = calloc(1, 1);
  slice->msg.slice_conf.ul.sched_name[0] = '\0';


  gNB_MAC_INST *mac = RC.nrmac[0];
  AssertFatal(mac != NULL, "MAC instance not initialized\n");

  NR_SCHED_LOCK(&mac->sched_lock);

  /* Count number of connected UEs */
  uint32_t ue_count = 0;

  UE_iterator(mac->UE_info.connected_ue_list, UE) {
    ue_count++;
  }

  /* Allocate UE array */
  slice->msg.ue_slice_conf.len_ue_slice = ue_count;

  if (ue_count > 0) {
    slice->msg.ue_slice_conf.ues =
        calloc(ue_count, sizeof(ue_slice_assoc_t));
    AssertFatal(slice->msg.ue_slice_conf.ues != NULL,
                "Memory allocation failed\n");
  }

  /* Fill UE stats */
  uint32_t idx = 0;

  UE_iterator(mac->UE_info.connected_ue_list, UE) {

    slice->msg.ue_slice_conf.ues[idx].rnti =
        UE->rnti;

    slice->msg.ue_slice_conf.ues[idx].dl_id =
        UE->UE_sched_ctrl.xapp_sst;

    slice->msg.ue_slice_conf.ues[idx].ul_id = 0;

    slice->msg.ue_slice_conf.ues[idx].dl_prbs =
        UE->mac_stats.dl.current_rbs;

    slice->msg.ue_slice_conf.ues[idx].dl_bytes =
        UE->mac_stats.dl.current_bytes;

    idx++;
  }

  /* Generate timestamp in microseconds */
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  slice->msg.tstamp =
      (int64_t)ts.tv_sec * 1000000LL +
      (int64_t)ts.tv_nsec / 1000LL;


  NR_SCHED_UNLOCK(&mac->sched_lock);

  return true;
}


void read_slice_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == SLICE_AGENT_IF_E2_SETUP_ANS_V0 );

  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_slice_sm(void const* data)
{
  assert(data != NULL);
//  assert(data->type == SLICE_CTRL_REQ_V0);

  slice_ctrl_req_data_t const* slice_req_ctrl = (slice_ctrl_req_data_t const* )data; // &data->slice_req_ctrl;
  slice_ctrl_msg_t const* msg = &slice_req_ctrl->msg;

  if(msg->type == SLICE_CTRL_SM_V0_ADD){

    printf("[E2 Agent]: SLICE CONTROL ADD rx\n");

    // Access MAC instance
    gNB_MAC_INST *mac = RC.nrmac[0];
    AssertFatal(mac != NULL, "MAC instance not initialized\n");

    ul_dl_slice_conf_t const* dl = &msg->u.add_mod_slice.dl;

    NR_SCHED_LOCK(&mac->sched_lock);
    
    /* Reset all previous slice shares */
    for(int i = 0; i < 256; i++)
      mac->xapp_sst_prb_share[i] = 0.0f;

    for(uint32_t i = 0; i < dl->len_slices; i++) {

        fr_slice_t const* s = &dl->slices[i];

        int slice_id = s->id;

        // Only handle NVS capacity mode (percentage-based slicing)
        if(s->params.type == SLICE_ALG_SM_V0_NVS &&
           s->params.u.nvs.conf == SLICE_SM_NVS_V0_CAPACITY) {

            float ratio = s->params.u.nvs.u.capacity.u.pct_reserved;

            if (slice_id >= 0 && slice_id < 256) {
                mac->xapp_sst_prb_share[slice_id] = ratio;
                printf("[E2 Agent] Updated SST %d PRB share = %.3f\n",
                       slice_id, ratio);
            } else {
                printf("[E2 Agent] Invalid SST %d received — ignored\n", slice_id);
            }

        }
    }

    NR_SCHED_UNLOCK(&mac->sched_lock);
  } else if (msg->type == SLICE_CTRL_SM_V0_DEL){
    printf("[E2 Agent]: SLICE CONTROL DEL rx\n");
  } else if (msg->type == SLICE_CTRL_SM_V0_UE_SLICE_ASSOC){

  printf("[E2 Agent]: SLICE CONTROL ASSOC rx\n");

  gNB_MAC_INST *mac = RC.nrmac[0];
  AssertFatal(mac != NULL, "MAC instance not initialized\n");

  NR_SCHED_LOCK(&mac->sched_lock);

  for(uint32_t i = 0; i < msg->u.ue_slice.len_ue_slice; i++) {

    uint16_t rnti = msg->u.ue_slice.ues[i].rnti;
    uint8_t slice_id = msg->u.ue_slice.ues[i].dl_id;

    // Find UE manually (no find_nr_UE helper needed)
    NR_UE_info_t *UE = NULL;

    UE_iterator(mac->UE_info.connected_ue_list, tmpUE) {
      if (tmpUE->rnti == rnti) {
        UE = tmpUE;
        break;
      }
    }

    if(UE) {
      UE->UE_sched_ctrl.xapp_sst = slice_id;

      printf("[E2 Agent] UE 0x%04x associated to SST %d\n",
             rnti, slice_id);
    } else {
      printf("[E2 Agent] UE 0x%04x not found!\n", rnti);
    }
  }

  NR_SCHED_UNLOCK(&mac->sched_lock);
} else {
    assert(0!=0 && "Unknown msg_type!");
  }

  sm_ag_if_ans_t ans = {.type = CTRL_OUTCOME_SM_AG_IF_ANS_V0};
  ans.ctrl_out.type = SLICE_AGENT_IF_CTRL_ANS_V0;
  return ans;

}


