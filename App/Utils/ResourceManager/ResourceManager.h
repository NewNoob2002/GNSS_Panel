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
#ifndef __RESOURCE_MANAGER_H
#define __RESOURCE_MANAGER_H

#include <string>
#include <vector>

class ResourceManager {

  public:
    ResourceManager();
    ~ResourceManager();

    bool add_resource(const char* name, void* ptr);
    bool remove_resource(const char* name);
    void* get_resource(const char* name) const;
    void set_default(void* ptr);

  private:
    typedef struct ResourceNode {
        std::string name;
        void* ptr;

        bool operator==(const struct ResourceNode& n) const {
            return this->name == n.name && this->ptr == n.ptr;
        }
    } ResourceNode_t;

  private:
    std::vector<ResourceNode_t> node_pool_;
    void* default_ptr_;
    bool search_node(const char* name, ResourceNode_t* node) const;
};

#endif
