/*
 Itay Marom
 Cisco Systems, Inc.
*/

/*
Copyright (c) 2015-2015 Cisco Systems, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <trex_vm_splitter.h>
#include <trex_stateless.h>

void
TrexVmSplitter::split(TrexStream *stream, std::vector<TrexStream *> core_streams) {

    /* nothing to do if no VM */
    if (stream->m_vm.is_vm_empty()) {
        return;
    }

    /* prepare some vars */
    m_dp_core_count = core_streams.size();
    m_core_streams  = &core_streams;
    m_stream        = stream;

    /* if we cannot split - compile the main and duplicate */
    bool rc = split_internal();
    if (!rc) {

        /* compile the stream and simply clone it to all streams */
        m_stream->vm_compile();

        /* for every core - simply clone the DP object */
        for (TrexStream *core_stream : *m_core_streams) {
            core_stream->m_vm_dp = m_stream->m_vm_dp->clone();
        }

        /* no need for the reference stream DP object */
        delete m_stream->m_vm_dp;
        m_stream->m_vm_dp = NULL;
    }
}

bool
TrexVmSplitter::split_internal() {

    const StreamVmInstruction *split_instr = m_stream->m_vm.get_split_instruction();
    if (split_instr == NULL) {
        return false;
    }

    if (split_instr->get_instruction_type() == StreamVmInstruction::itFLOW_MAN) {
        return split_by_flow_var( (const StreamVmInstructionFlowMan *)split_instr );

    } else if (split_instr->get_instruction_type() == StreamVmInstruction::itFLOW_CLIENT) {
        return split_by_flow_client_var( (const StreamVmInstructionFlowClient *)split_instr );

    } else {
        throw TrexException("VM splitter : cannot split by instruction which is not flow var or flow client var");
    }

}

/**
 * split VM by flow var 
 * 
 * @author imarom (20-Dec-15)
 * 
 * @param instr 
 * 
 * @return bool 
 */
bool
TrexVmSplitter::split_by_flow_var(const StreamVmInstructionFlowMan *instr) {
    /* no point in splitting random */
    if (instr->m_op == StreamVmInstructionFlowMan::FLOW_VAR_OP_RANDOM) {
        return false;
    }

    /* if the range is too small - multiply  */
    if (instr->get_range() < m_dp_core_count) {
        // FIXME
        return false;
    }

    /* we need to split - duplicate VM now */
    duplicate_vm();

    /* calculate range splitting */
    uint64_t range = instr->get_range();

    uint64_t range_part = range / m_dp_core_count;
    uint64_t leftover   = range % m_dp_core_count;

    /* first core handles a bit more */
    uint64_t start   = instr->m_min_value;
    uint64_t end     = start + range_part + leftover - 1;


    /* do work */
    for (TrexStream *core_stream : *m_core_streams) {

        /* get the per-core instruction to split */
        StreamVmInstructionFlowMan *per_core_instr = (StreamVmInstructionFlowMan *)core_stream->m_vm.get_split_instruction();

        per_core_instr->m_min_value  = start;
        per_core_instr->m_max_value  = end; 

        /* init value is the max value because the VM program's first iteration */
        per_core_instr->m_init_value = end;

        core_stream->vm_compile();

        start = end + 1;
        end   = start + range_part - 1;
    }

    return true;
}


bool
TrexVmSplitter::split_by_flow_client_var(const StreamVmInstructionFlowClient *instr) {
    return false;
}


void
TrexVmSplitter::duplicate_vm() {
    /* for each core - duplicate the instructions */
    for (TrexStream *core_stream : *m_core_streams) {
        m_stream->m_vm.copy_instructions(core_stream->m_vm);
    }
}
