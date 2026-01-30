#pragma once  

#include "json.h"  
#include <stack>  
#include <string>  
#include <stdexcept>  
#include <memory>  

namespace json {

    class Builder {
    public:


        class BaseContext;
        class ArrayContext;
        class DictContext;
        class KeyContext;

        Builder() = default;

        Builder& Value(Node::Value value);
        Builder& Key(std::string key);
        Builder& EndDict();
        Builder& EndArray();
        Node Build();

        class BaseContext {
        public:
            explicit BaseContext(Builder& builder) : builder_(builder) {}

            Builder& Value(Node::Value value) { return builder_.Value(std::move(value)); }
            Builder& Key(std::string key) { return builder_.Key(std::move(key)); }
            Builder& EndDict() { return builder_.EndDict(); }
            Builder& EndArray() { return builder_.EndArray(); }

        protected:
            Builder& builder_;
        };

        class KeyContext;
        class DictContext : public BaseContext {
        public:
            using BaseContext::BaseContext;
            KeyContext Key(std::string key);
            Builder& EndDict();
        };

        class KeyContext : public BaseContext {
        public:
            using BaseContext::BaseContext;
            DictContext Value(Node::Value value);
            DictContext StartDict();
            ArrayContext StartArray();
        };

        class ArrayContext : public BaseContext {
        public:
            using BaseContext::BaseContext;
            ArrayContext Value(Node::Value value);
            DictContext StartDict();
            ArrayContext StartArray();
            Builder& EndArray();
        };

        DictContext StartDict();
        ArrayContext StartArray();

    private:
        void AddValue(Node::Value value);
        void AddKey(std::string key);
        void StartDictImpl();
        void StartArrayImpl();
        void EndDictImpl();
        void EndArrayImpl();

        enum class State { EMPTY, ARRAY, DICT, KEY };

        Node root_;
        std::stack<State> state_stack_;
        std::stack<Node*> node_stack_;
        std::string current_key_;
        bool built_ = false;

        void CheckState(State expected, const std::string& method_name);
        void CheckNotBuilt();
        Node* CurrentNode();
    };


} // namespace json