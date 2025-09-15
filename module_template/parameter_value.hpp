/**Contains a class for settins/state varibles of different types**/

#pragma once

class ParameterValue {
public:
  enum Type { TYPE_INT, TYPE_BOOL, TYPE_STRING, TYPE_NONE };

  ParameterValue() : type(TYPE_NONE), intValue(0), boolValue(false), strValue() {}
  ParameterValue(int v) : type(TYPE_INT), intValue(v), boolValue(false), strValue() {}
  ParameterValue(bool v) : type(TYPE_BOOL), intValue(0), boolValue(v), strValue() {}
  ParameterValue(const String &v) : type(TYPE_STRING), intValue(0), boolValue(false), strValue(v) {}
  ParameterValue(const char *v) : type(TYPE_STRING), intValue(0), boolValue(false), strValue(v) {}

  Type get_type() const { return type; }

  // getters with simple validity checks
  int as_int(bool &ok) const {
    ok = (type == TYPE_INT);
    return ok ? intValue : 0;
  }
  int as_int() const {
    assert(type == TYPE_INT);
    return intValue;
  }
  bool as_bool(bool &ok) const {
    ok = (type == TYPE_BOOL);
    return ok ? boolValue : false;
  }
  bool as_bool() const {
    assert(type == TYPE_BOOL);
    return boolValue;
  }
  String as_string(bool &ok) const {
    ok = (type == TYPE_STRING);
    return ok ? strValue : String();
  }
  String as_string() const {
    assert(type == TYPE_STRING);
    return strValue;
  }

  // setters
  void set_int(int v) { clear(); type = TYPE_INT; intValue = v; }
  void set_bool(bool v) { clear(); type = TYPE_BOOL; boolValue = v; }
  void set_string(const String &v) { clear(); type = TYPE_STRING; strValue = v; }

private:
  void clear() {
    // For now only String needs explicit clear — but using String's assignment handles it.
    if (type == TYPE_STRING) strValue = String();
    type = TYPE_NONE;
    intValue = 0;
    boolValue = false;
  }

  Type type;
  // TODO: use union instead separate members?
  int intValue;
  bool boolValue;
  String strValue;
};
