#include "renderer_internal.h"

namespace pulse_renderer_internal {
    // Acquire a free cached buffer with at least `size` bytes.
    uint8_t* UboBlockCache::acquire(uint32_t size, uint32_t frame_index, uint32_t& out_block_size) {
        int best = -1;
        for (size_t i = 0; i < blocks_.size(); ++i) {
            auto& block = blocks_[i];
            if (block.in_use || block.size < size) continue;
            if (best < 0 || block.size < blocks_[best].size)
                best = static_cast<int>(i);
        }

        if (best >= 0) {
            auto& block = blocks_[best];
            block.in_use = true;
            block.last_used_frame = frame_index;
            out_block_size = block.size;
            return block.data;
        }

        uint32_t block_size = size > kMinBlockSize ? size : kMinBlockSize;
        auto* data = new uint8_t[block_size];
        blocks_.push_back({ data, block_size, frame_index, true });
        out_block_size = block_size;
        return data;
    }

    // Called after all per-view GpuBlocks for this frame were destroyed.
    void UboBlockCache::frame_end() {
        for (auto& block : blocks_)
            block.in_use = false;
    }

    // Free blocks that have not been used for kMaxIdleFrames.
    void UboBlockCache::release_idle(uint32_t current_frame) {
        for (auto it = blocks_.begin(); it != blocks_.end();) {
            if (!it->in_use &&
                current_frame > it->last_used_frame &&
                current_frame - it->last_used_frame > kMaxIdleFrames) {
                delete[] it->data;
                it = blocks_.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void UboBlockCache::reset() {
        for (auto& block : blocks_)
            delete[] block.data;
        blocks_.clear();
    }
}
