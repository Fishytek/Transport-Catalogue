#include "json_builder.h" 

namespace json {

    // ��������������� ������ Builder
    void Builder::CheckState(State expected, const std::string& method_name) {
        if (state_stack_.empty() || state_stack_.top() != expected) {
            throw std::logic_error(method_name + " called in wrong context");
        }
    }

    void Builder::CheckNotBuilt() {
        if (built_) {
            throw std::logic_error("Build already called");
        }
    }

    Node* Builder::CurrentNode() {
        if (node_stack_.empty()) {
            throw std::logic_error("No current container");
        }
        return node_stack_.top();
    }

    void Builder::AddKey(std::string key) {
        CheckNotBuilt();
        CheckState(State::DICT, "Key");
        if (!current_key_.empty()) {
            throw std::logic_error("Key already set");
        }
        current_key_ = std::move(key);
        state_stack_.push(State::KEY);
    }

    void Builder::AddValue(Node::Value value) {
        CheckNotBuilt();

        Node node;
        node.GetValue() = std::move(value);

        if (state_stack_.empty()) {
            if (!root_.IsNull()) {
                throw std::logic_error("Multiple root values");
            }
            root_ = std::move(node);
            return;
        }

        Node* current = CurrentNode();
        auto state = state_stack_.top();

        if (state == State::ARRAY) {
            std::get<Array>(current->GetValue()).emplace_back(std::move(node));
        }
        else if (state == State::KEY) {
            state_stack_.pop();
            std::get<Dict>(current->GetValue())[current_key_] = std::move(node);
            current_key_.clear();
        }
        else {
            throw std::logic_error("Invalid state for Value");
        }
    }

    Builder::DictContext Builder::StartDict() {
        StartDictImpl();
        return DictContext(*this);
    }

    void Builder::StartDictImpl() {
        CheckNotBuilt();
        Node new_node;
        new_node.GetValue() = Dict{};

        if (state_stack_.empty()) {
            if (!root_.IsNull()) throw std::logic_error("Multiple root values");
            root_ = std::move(new_node);
            state_stack_.push(State::DICT);
            node_stack_.push(&root_);
            return;
        }

        Node* current = CurrentNode();
        auto state = state_stack_.top();

        if (state == State::ARRAY) {
            auto& arr = std::get<Array>(current->GetValue());
            arr.emplace_back(std::move(new_node));
            node_stack_.push(&arr.back());
        }
        else if (state == State::KEY) {
            state_stack_.pop();
            std::get<Dict>(current->GetValue())[current_key_] = std::move(new_node);
            node_stack_.push(&std::get<Dict>(current->GetValue())[current_key_]);
            current_key_.clear();
        }
        else {
            throw std::logic_error("StartDict in invalid state");
        }
        state_stack_.push(State::DICT);
    }

    Builder::ArrayContext Builder::StartArray() {
        StartArrayImpl();
        return ArrayContext(*this);
    }

    void Builder::StartArrayImpl() {
        CheckNotBuilt();
        Node new_node;
        new_node.GetValue() = Array{};

        if (state_stack_.empty()) {
            if (!root_.IsNull()) throw std::logic_error("Multiple root values");
            root_ = std::move(new_node);
            state_stack_.push(State::ARRAY);
            node_stack_.push(&root_);
            return;
        }

        Node* current = CurrentNode();
        auto state = state_stack_.top();

        if (state == State::ARRAY) {
            auto& arr = std::get<Array>(current->GetValue());
            arr.emplace_back(std::move(new_node));
            node_stack_.push(&arr.back());
        }
        else if (state == State::KEY) {
            state_stack_.pop();
            std::get<Dict>(current->GetValue())[current_key_] = std::move(new_node);
            node_stack_.push(&std::get<Dict>(current->GetValue())[current_key_]);
            current_key_.clear();
        }
        else {
            throw std::logic_error("StartArray in invalid state");
        }
        state_stack_.push(State::ARRAY);
    }

    void Builder::EndDictImpl() {
        CheckNotBuilt();
        CheckState(State::DICT, "EndDict");
        state_stack_.pop();
        node_stack_.pop();
    }

    void Builder::EndArrayImpl() {
        CheckNotBuilt();
        CheckState(State::ARRAY, "EndArray");
        state_stack_.pop();
        node_stack_.pop();
    }

    Builder& Builder::EndDict() {
        EndDictImpl();
        return *this;
    }

    Builder& Builder::EndArray() {
        EndArrayImpl();
        return *this;
    }

    Node Builder::Build() {
        CheckNotBuilt();
        if (!state_stack_.empty() || !node_stack_.empty()) {
            throw std::logic_error("Build called on incomplete JSON");
        }
        if (root_.IsNull()) {
            throw std::logic_error("No root node set");
        }
        built_ = true;
        return root_;
    }

    // ���������� ������� ����������
    Builder::KeyContext Builder::DictContext::Key(std::string key) {
        builder_.AddKey(std::move(key));
        return KeyContext(builder_);
    }

    Builder& Builder::DictContext::EndDict() {
        builder_.EndDict();
        return builder_;
    }

    Builder::DictContext Builder::KeyContext::Value(Node::Value value) {
        builder_.AddValue(std::move(value));
        return DictContext(builder_);
    }

    Builder::DictContext Builder::KeyContext::StartDict() {
        return builder_.StartDict();
    }

    Builder::ArrayContext Builder::KeyContext::StartArray() {
        return builder_.StartArray();
    }

    Builder::ArrayContext Builder::ArrayContext::Value(Node::Value value) {
        builder_.AddValue(std::move(value));
        return ArrayContext(builder_);
    }

    Builder::DictContext Builder::ArrayContext::StartDict() {
        return builder_.StartDict();
    }

    Builder::ArrayContext Builder::ArrayContext::StartArray() {
        return builder_.StartArray();
    }

    Builder& Builder::ArrayContext::EndArray() {
        return builder_.EndArray();
    }

} // namespace json