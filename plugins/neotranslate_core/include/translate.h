#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <optional>
#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <functional>

struct Utf8Array {
  const char8_t* begin;
  const char8_t* end;
};

class LanPair {
public:
  double& from;
  double& to;
public:
  explicit LanPair(class YJson& array);
  auto f() const { return static_cast<size_t>(from); }
  auto t() const { return static_cast<size_t>(to); } 
};

class Translate {
public:
  // enum class Lan { AUTO, ZH_CN, ZH_TW, EN_US, JA_JP, FR_LU, RU_RU, MAX };
  enum Source { Baidu = 0, Youdao = 1, None = 2 } m_Source;
  typedef std::vector<std::vector<std::pair<std::u8string, std::vector<std::u8string>>>> LanguageMap;
  typedef std::function<void(const void*, size_t)> Callback ;

public:
  explicit Translate(class YJson& setting, Callback&& callback);
  ~Translate();

public:
  template<typename _Utf8Array>
  bool GetResult(const _Utf8Array& text) {
    const Utf8Array array { 
      reinterpret_cast<const char8_t*>(text.data()),
      reinterpret_cast<const char8_t*>(text.data()) + text.size() };
    return m_Source == Youdao ? GetResultYoudao(array) : GetResultBaidu(array);
  }
  void SetSource(Source dict);
  inline Source GetSource() const { return m_Source; }
  std::optional<std::pair<int, int>> ReverseLanguage();

private:
  LanPair m_LanPairBaidu, m_LanPairYoudao;
  const Callback m_Callback;
public:
  LanPair* m_LanPair;
  static std::map<std::u8string, std::u8string> m_LangNameMap;
  static const LanguageMap m_LanguageCanFromTo;

private:
  bool GetResultBaidu(const Utf8Array& text);
  bool GetResultYoudao(const Utf8Array& text);
  void FormatYoudaoResult(const class YJson& data);
};

#endif  // TRANSLATE_H
