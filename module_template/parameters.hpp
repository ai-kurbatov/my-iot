/**Stores generilized parameters class for settings and current state. It stores strings with paramter names,
so it is pretty costy for memory.**/

#pragma once

#include <vector>
#include <utility>

#include <Arduino.h>

#include "parameter_value.hpp"


class Parameters {
public:
  using Pair = std::pair<String, ParameterValue>;

  Parameters() = default;

  /// Adds or updates paramter. Returns index of updated value. It is for initial setting only! Use [] for faster update
  size_t add(const String &name, const ParameterValue &value) {
    int idx = index_of(name);
    if (idx >= 0) {
      items[idx].second = value;
      return idx;
    } else {
      items.emplace_back(name, value);
      return items.size() - 1;
    }
  }

  /// Set by name. If not found does nothing
  void set(const String &name, const ParameterValue &value) {
    int idx = index_of(name);
    if (idx >= 0) 
      items[idx].second = value;
  }


  /// check existence
  bool has(const String &name) const {
    return index_of_const(name) >= 0;
  }

  /// index-based operator[] (mutable)
  ParameterValue& operator[](size_t idx) {
    assert(idx < items.size());
    return items[idx].second;
  }
  /// index-based operator[] (const)
  const ParameterValue& operator[](size_t idx) const {
    assert(idx < items.size());
    return items[idx].second;
  }

  /// access pair by index
  Pair& pair_at(size_t idx) {
    assert(idx < items.size());
    return items[idx];
  }
  const Pair& pair_at(size_t idx) const {
    assert(idx < items.size());
    return items[idx];
  }

  /// size and access by index
  size_t size() const { return items.size(); }

  /// clear all
  void clear() { items.clear(); }


  /// produce JSON string of all parameters
  String to_json() const {
    String json;
    json.reserve(128 + items.size() * 32);
    json += "{";
    for (size_t i = 0; i < items.size(); ++i) {
      if (i) json += ",";
      // name
      json += "\"";
      json += escape_string(items[i].first);
      json += "\":";
      // value by type
      const ParameterValue &pv = items[i].second;
      switch (pv.getType()) {
        case ParameterValue::TYPE_INT: {
          bool ok;
          int v = pv.asInt(ok);
          if (!ok) json += "null";
          else {
            char buf[16];
            sprintf(buf, "%d", v);
            json += buf;
          }
          break;
        }
        case ParameterValue::TYPE_BOOL: {
          bool ok;
          bool v = pv.asBool(ok);
          json += (ok && v) ? "true" : (ok ? "false" : "null");
          break;
        }
        case ParameterValue::TYPE_STRING: {
          bool ok;
          String v = pv.asString(ok);
          if (!ok) json += "null";
          else {
            json += "\"";
            json += escape_string(v);
            json += "\"";
          }
          break;
        }
        case ParameterValue::TYPE_NONE:
        default:
          json += "null";
          break;
      }
    }
    json += "}";
    return json;
  } 

  // helper: find index by name (-1 if not found)
  int index_of(const String &name) {
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i].first == name) return (int)i;
    }
    return -1;
  }
  int index_of_const(const String &name) const {
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i].first == name) return (int)i;
    }
    return -1;
  }

private:
  /// escape JSON special chars in an Arduino String
  static String escape_string(const String &s) {
    String out;
    out.reserve(s.length() + 16);
    for (size_t i = 0; i < s.length(); ++i) {
      char c = s[i];
      switch (c) {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
          if ((uint8_t)c < 0x20) {
            // control char -> \u00XX
            char buf[7];
            sprintf(buf, "\\u%04x", (uint8_t)c);
            out += buf;
          } else {
            out += c;
          }
      }
    }
    return out;
  }

  std::vector<Pair> items;
};