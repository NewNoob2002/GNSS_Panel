/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "ResourceManager.h"
#include <algorithm>
#include <cstring>
#include "lvgl.h"

#define RES_LOG_INFO  LV_LOG_INFO
#define RES_LOG_WARN  LV_LOG_WARN
#define RES_LOG_ERROR LV_LOG_ERROR

ResourceManager::ResourceManager() {
    default_ptr_ = nullptr;
}

ResourceManager::~ResourceManager() = default;

/**
  * @brief  Search resource node based on name
  * @param  name: Resource Name
  * @param  node: Pointer to the resource node
  * @retval Return true if the search is successful
  */
bool ResourceManager::search_node(const char* name, ResourceNode_t* node) const {
    if (name == nullptr || node == nullptr)
        return false;

    for (const auto& iter : node_pool_) {
        if (iter.name == name) {
            *node = iter;
            return true;
        }
    }
    return false;
}

/**
  * @brief  Add resources to the resource pool
  * @param  name: Resource Name
  * @param  ptr: Pointer to the resource
  * @retval Return true if the addition is successful
  */
bool ResourceManager::add_resource(const char* name, void* ptr) {
    if (name == nullptr || ptr == nullptr)
        return false;

    ResourceNode_t node;
    if (search_node(name, &node)) {
        RES_LOG_WARN("Resource: %s was register", name);
        return false;
    }

    node.name = name;
    node.ptr = ptr;
    node_pool_.push_back(node);

    RES_LOG_INFO("Resource: %s[0x%p] add success", node.name.c_str(), node.ptr);

    return true;
}

/**
  * @brief  Remove resources from the resource pool
  * @param  name: Resource Name
  * @retval Return true if the removal is successful
  */
bool ResourceManager::remove_resource(const char* name) {
    if (name == nullptr)
        return false;

    ResourceNode_t node;
    if (!search_node(name, &node)) {
        RES_LOG_ERROR("Resource: %s was not found", name);
        return false;
    }

    auto iter = std::find(node_pool_.begin(), node_pool_.end(), node);

    if (iter == node_pool_.end()) {
        RES_LOG_ERROR("Resource: %s was not found", name);
        return false;
    }

    node_pool_.erase(iter);

    RES_LOG_INFO("Resource: %s remove success", name);

    return true;
}

/**
  * @brief  Get resource address
  * @param  name: Resource Name
  * @retval If the acquisition is successful, return the address of the resource, otherwise return the default resource
  */
void* ResourceManager::get_resource(const char* name) const {
    if (name == nullptr)
        return default_ptr_;

    ResourceNode_t node;

    if (!search_node(name, &node)) {
        RES_LOG_WARN("Resource: %s was not found, return default[0x%p]", name, default_ptr_);
        return default_ptr_;
    }

    RES_LOG_INFO("Resource: %s[0x%p] was found", name, node.ptr);

    return node.ptr;
}

/**
  * @brief  Set default resources
  * @param  ptr: Pointer to the default resource
  * @retval None
  */
void ResourceManager::set_default(void* ptr) {
    default_ptr_ = ptr;
    RES_LOG_INFO("Resource: set [0x%p] to default", default_ptr_);
}
