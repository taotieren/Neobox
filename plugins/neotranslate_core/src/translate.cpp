#include <translate.h>
#include <yjson.h>
#include <httplib.h>
#include <sha256.h>

#include <chrono>
#include <format>
#include <random>

using namespace std::literals;
inline std::u8string GetTimeStamp() {
  using namespace std::chrono;
  const auto ms =
      duration_cast<seconds>(system_clock::now().time_since_epoch());
  const auto msCount = std::to_string(ms.count());
  return std::u8string(msCount.begin(), msCount.end());
}

inline std::u8string Truncate(std::u8string q) {
  if (q.size() > 20) {
    std::string result = std::to_string(q.size());
    q.replace(q.begin() + 10, q.end() - 10, result.begin(), result.end());
  }
  return q;
}

inline std::u8string Sha256(const std::u8string& str) {
		SHA256 sha;
		sha.update(str);
		uint8_t * digest = sha.digest();
		auto result =  SHA256::toString<char8_t>(digest);
		delete[] digest;
  return result;
}

inline std::u8string Uuid1() {
  std::u8string result(37, 0);
  union {
    struct {
      uint32_t time_low;
      uint16_t time_mid;
      uint16_t time_hi_and_version;
      uint8_t clk_seq_hi_res;
      uint8_t clk_seq_low;
      uint8_t node[6];
    };
    uint16_t __rnd[8];
  } uuid;

  std::independent_bits_engine<std::default_random_engine, 16, uint16_t> bytes(
      std::default_random_engine{}());
  std::generate_n(uuid.__rnd, 8, std::ref(bytes));

  // Refer Section 4.2 of RFC-4122
  // https://tools.ietf.org/html/rfc4122#section-4.2
  uuid.clk_seq_hi_res = (uint8_t)((uuid.clk_seq_hi_res & 0x3F) | 0x80);
  uuid.time_hi_and_version =
      (uint16_t)((uuid.time_hi_and_version & 0x0FFF) | 0x4000);

  snprintf(reinterpret_cast<char*>(&result.front()), 37,
           "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid.time_low,
           uuid.time_mid, uuid.time_hi_and_version, uuid.clk_seq_hi_res,
           uuid.clk_seq_low, uuid.node[0], uuid.node[1], uuid.node[2],
           uuid.node[3], uuid.node[4], uuid.node[5]);
  result.pop_back();
  return result;
}

Translate::Translate()
    : m_LanPair(0, 0),
      m_LanMap({
          {u8"auto", u8"自动检测"},
          {u8"zh", u8"中文简体"},
          {u8"zh-CNS", u8"中文简体"},
          {u8"cht", u8"中文繁体"},
          {u8"en", u8"英语"},
          {u8"ja", u8"日语"},
          {u8"jp", u8"日语"},
          {u8"fra", u8"法语"},
          {u8"fr", u8"法语"},
          {u8"ko", u8"韩语"},
          {u8"ru", u8"俄语"},
      }),
      m_LanNamesBaidu(
          {{u8"auto", {u8"zh", u8"cht", u8"en", u8"jp", u8"fra", u8"ru"}},
           {u8"zh", {u8"cht", u8"en", u8"jp", u8"fra", u8"ru"}},
           {u8"cht", {u8"zh", u8"en", u8"jp", u8"fra", u8"ru"}},
           {u8"en", {u8"zh", u8"cht", u8"jp", u8"fra", u8"ru"}},
           {u8"jp", {u8"zh", u8"cht", u8"en", u8"fra", u8"ru"}},
           {u8"fra", {u8"zh", u8"cht", u8"en", u8"jp", u8"ru"}},
           {u8"ru", {u8"zh", u8"cht", u8"en", u8"jp", u8"fra"}}}),
      m_LanNamesYoudao({
          {u8"auto",
           {u8"auto", u8"zh-CNS", u8"en", u8"ja", u8"ko", u8"fr", u8"ru"}},
          {u8"zh-CNS", {u8"auto", u8"en", u8"ja", u8"ko", u8"fr", u8"ru"}},
          {u8"en", {u8"auto", u8"zh-CNS", u8"ja"}},
          {u8"ja", {u8"auto", u8"zh-CNS", u8"en"}},
          {u8"ko", {u8"auto", u8"zh-CNS"}},
          {u8"fr", {u8"auto", u8"zh-CNS"}},
          {u8"ru", {u8"auto", u8"zh-CNS"}},
      }) {
  SetDict(Dict::Baidu);
  // SetDict(Dict::Youdao);
}

Translate::~Translate() {}

void Translate::SetDict(Dict dict) {
  m_Dict = dict;
  m_LanPair = {0, 0};
}

std::u8string Translate::GetResultBaidu(const std::u8string& text) {
  static YJson jsData(
      YJson::O{{u8"from", u8"auto"}, {u8"to", u8"zh"}, {u8"q", YJson::String}});

  jsData[u8"from"] = m_LanNamesBaidu[m_LanPair.first].first;
  jsData[u8"to"] =
      m_LanNamesBaidu[m_LanPair.first].second[m_LanPair.second];
  jsData[u8"q"] = text;

  std::u8string strJsonData = jsData.toString(false);
  HttpLib clt("https://aip.baidubce.com/rpc/2.0/mt/texttrans/v1?"
                   "access_token=24.e722d8c3c3090cda4645507c8d1c06ba.2592000.1669883009.282335-27415445"s);
  clt.SetHeader("Content-Type", "application/json;charset=utf-8");
  clt.SetPostData(strJsonData.data(), strJsonData.size());

  auto res = clt.Get();

  if (res->status == 200 || res->status == 201) {
    YJson jsData(res->body.begin(), res->body.end());
    if (auto iter = jsData.find(u8"error_msg"); iter != jsData.endO()) {
      // Access token expired
      return iter->second.getValueString();
    }
    const auto& obTransResult =
        jsData[u8"result"][u8"trans_result"].getArray();

    std::u8string content;
    for (auto& item : obTransResult) {
      content.append(item[u8"dst"].getValueString());
      content.push_back('\n');
    }
    return content;
  }
  return std::u8string();
}

std::u8string Translate::GetResultYoudao(const std::u8string& text) {
  using namespace std::literals;
  static const char8_t APP_KEY[]{u8"0b5f90d14623b917"};
  static const char8_t APP_SECRET[]{u8"8X1HcIvXXETCRf2smIbey8AGJ2xGRyK3"};
  static YJson m_scJson(YJson::O{{u8"from"sv, u8"auto"sv},
                                 {u8"to"sv, u8"auto"sv},
                                 {u8"appKey"sv, APP_KEY},
                                 {u8"signType"sv, u8"v3"sv},
                                 {u8"curtime"sv, YJson::String},
                                 {u8"q"sv, YJson::Null},
                                 {u8"salt"sv, YJson::Null},
                                 {u8"sign"sv, YJson::Null}});

  m_scJson[u8"from"] = m_LanNamesYoudao[m_LanPair.first].first;
  m_scJson[u8"to"] =
      m_LanNamesYoudao[m_LanPair.first].second[m_LanPair.second];

  m_scJson[u8"q"] = text;
  const std::u8string&& curtime = GetTimeStamp();
  m_scJson[u8"curtime"].setText(curtime);
  const std::u8string&& salt =
      Uuid1();  //"d818bc30-99df-11ec-9e18-1cbfc0a98096";
  m_scJson[u8"salt"] = salt;
  m_scJson[u8"sign"] =
      Sha256(APP_KEY + Truncate(text) + salt + curtime + APP_SECRET);

  HttpLib clt(m_scJson.urlEncode(u8"http://openapi.youdao.com/api/?"));
  clt.SetHeader("Content-Type", "application/x-www-form-urlencoded");

  std::u8string content;
  auto res = clt.Get();
  if (res->status == 200 || res->status == 201) {
    YJson jsData(res->body.begin(), res->body.end());
    if (jsData[u8"errorCode"].getValueString() != u8"0") {
      return jsData[u8"errorCode"].getValueString();
    }
    FormatYoudaoResult(content, jsData);
    return content;
  }
  return content;
}

std::u8string Translate::GetResult(const std::u8string& text) {
  std::u8string result;
  switch (m_Dict) {
    case Dict::Baidu:
      result = GetResultBaidu(text);
      break;
    case Dict::Youdao:
      result = GetResultYoudao(text);
      break;
    default:
      break;
  }
  return result;
}

inline std::string S(const std::u8string& x) {
  return std::string(x.begin(), x.end());
}

void Translate::FormatYoudaoResult(std::u8string& result, const YJson& data) {
  using namespace std::literals;
  std::string html;
  // std::cout << data;
  std::u8string l = data[u8"l"].getValueString();
  if (auto basic = data.find(u8"basic"); basic == data.endO()) {
    html = std::format("<p>{}</p><hr/>",
                       S(data[u8"query"].getValueString()));
    auto& translation = data[u8"translation"].getArray();
    std::string buffer;
    if (!translation.empty()) {
      for (auto& i : translation) {
        buffer.append(i.getValueString().begin(), i.getValueString().end());
        buffer.append("; "s);
      }
      buffer.erase(buffer.size() - 2);
      html.append(std::format("<p>- 释义：{}</p><hr/>", buffer));
    }
  } else {
    if (!l.compare(0, 6, u8"zh-CNS")) {
      html = std::format(
          "<p>{}  英[<span style='color:#44EEEE;'>{}</span>] 美[<span "
          "style='color:#44EEEE;'>{}</span>]</p><hr/>",
          S(data[u8"query"].getValueString()),
          S(basic->second[u8"uk-phonetic"].getValueString()),
          S(basic->second[u8"us-phonetic"].getValueString()));
    } else if (auto phonetic = basic->second.find(u8"phonetic");
               phonetic != basic->second.endO()) {
      html = std::format(
          "<p>{}  拼音[<span style='color:#44EEEE;'>{}</span>]</p><hr/>",
          S(data[u8"query"].getValueString()),
          S(basic->second[u8"phonetic"].getValueString()));
    } else {
      html = std::format("<p>{}</p><hr/>",
                         S(data[u8"query"].getValueString()));
    }
    auto wfs = basic->second.find(u8"wfs"sv);
    if (wfs != basic->second.endO()) {
      for (auto& wf : wfs->second.getArray()) {
        auto temp = wf.beginO();
        if (temp == wf.endO() || temp->first != u8"wf"sv)
          break;
        html.append(std::format(
            "<p>+ {}：<span style='color:#FF77CC;'>{}</span></p>",
            S(temp->second.find(u8"name")->second.getValueString()),
            S(temp->second.find(u8"value")->second.getValueString())));
      }
      html.append("<hr/>"s);
    }

    const auto& explains = basic->second.find(u8"explains")->second.getArray();
    for (auto& i : explains) {
      html.append(std::format("<p>- {}</p>", S(i.getValueString())));
    }
    if (!explains.empty())
      html.append("<hr/>"s);
  }

  if (auto ptr = data.find(u8"web"); ptr != data.endO()) {
    std::string buffer;
    for (int index = 0; auto& i : ptr->second.getArray()) {
      buffer.clear();
      const auto& value = i[u8"value"].getArray();
      for (auto& j : value) {
        buffer.append(j.getValueString().begin(), j.getValueString().end());
        buffer.append("; "s);
      }
      if (!buffer.empty())
        buffer.erase(buffer.size() - 2);
      html.append(std::format(
          "<p>{}. <span style='color: #FF00FF;'>{}</span> &lt;{}&gt;</p>",
          ++index, S(i[u8"key"].getValueString()), buffer));
    }
    html.append("<hr/>"s);
  }
  result.assign(html.begin(), html.end());
}