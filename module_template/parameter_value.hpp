/**Contains a class for settins/state varibles of different types**/

class ParameterValue {
public:
  enum Type { TYPE_INT, TYPE_BOOL, TYPE_STRING, TYPE_NONE };

  ParameterValue() : type(TYPE_NONE), intValue(0), boolValue(false), strValue() {}
  ParameterValue(int v) : type(TYPE_INT), intValue(v), boolValue(false), strValue() {}
  ParameterValue(bool v) : type(TYPE_BOOL), intValue(0), boolValue(v), strValue() {}
  ParameterValue(const String &v) : type(TYPE_STRING), intValue(0), boolValue(false), strValue(v) {}
  ParameterValue(const char *v) : type(TYPE_STRING), intValue(0), boolValue(false), strValue(v) {}

  Type getType() const { return type; }

  // getters with simple validity checks
  int asInt(bool &ok) const {
    ok = (type == TYPE_INT);
    return ok ? intValue : 0;
  }
  int asInt() const {
    assert(type == TYPE_INT);
    return intValue;
  }
  bool asBool(bool &ok) const {
    ok = (type == TYPE_BOOL);
    return ok ? boolValue : false;
  }
  bool asBool() const {
    assert(type == TYPE_BOOL);
    return boolValue;
  }
  String asString(bool &ok) const {
    ok = (type == TYPE_STRING);
    return ok ? strValue : String();
  }
  String asString() const {
    assert(type == TYPE_STRING);
    return strValue;
  }

  // setters
  void setInt(int v) { clear(); type = TYPE_INT; intValue = v; }
  void setBool(bool v) { clear(); type = TYPE_BOOL; boolValue = v; }
  void setString(const String &v) { clear(); type = TYPE_STRING; strValue = v; }

private:
  void clear() {
    // For now only String needs explicit clear — but using String's assignment handles it.
    if (type == TYPE_STRING) strValue = String();
    type = TYPE_NONE;
    intValue = 0;
    boolValue = false;
  }

  Type type;
  int intValue;
  bool boolValue;
  String strValue;
};
