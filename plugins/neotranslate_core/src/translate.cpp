#include <translate.h>
#include <yjson.h>
#include <httplib.h>
#include <sha256.h>
#include <md5.h>

#include <chrono>
#include <format>
#include <random>

using namespace std::literals;
namespace chrono = std::chrono;

static auto& operator<<(std::ostream& stream, const std::u8string& other) {
  return stream.write(reinterpret_cast<const char*>(other.data()), other.size());
}

inline std::u8string GetTimeStamp() {
  using namespace std::chrono;
  const auto ms =
      duration_cast<seconds>(system_clock::now().time_since_epoch());
  const auto msCount = std::to_string(ms.count());
  return std::u8string(msCount.begin(), msCount.end());
}

std::u8string Truncate(std::u8string q) {
  std::vector<char8_t const*> string;
  char8_t const* start = q.data(), *ptr = start, *stop = start + q.size();
  while (ptr < stop) {
    string.push_back(ptr);
    if (*ptr < 0x80) {
      ptr += 1;
    } else if (*ptr < 0xE0) {
      ptr += 2;
    } else if (*ptr < 0xF0) {
      ptr += 3;
    } else if (*ptr < 0xF8) {
      ptr += 4;
    } else if (*ptr < 0xFC) {
      ptr += 5;
    } else if (*ptr < 0xFE) {
      ptr += 6;
    } else {  // never
      ptr += 7;
    }
  }
  if (string.size() > 20) {
    std::string result = std::to_string(string.size());
    q.replace(string[10] - start + q.begin(),
      string[string.size() - 10] - start + q.begin(),
      result.begin(), result.end()
    );
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

inline std::u8string Md5(const std::u8string& str)
{
  return MD5(str).toStr<char8_t>();
}

inline std::u8string Uuid1() {
  std::string result;
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

  result = std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
      uuid.time_low, uuid.time_mid, uuid.time_hi_and_version, uuid.clk_seq_hi_res,
      uuid.clk_seq_low, uuid.node[0], uuid.node[1], uuid.node[2],
      uuid.node[3], uuid.node[4], uuid.node[5]);
  return std::u8string(result.begin(), result.end());
}


std::map<std::u8string, std::u8string> Translate::m_LangNameMap
{
  {u8"auto",   u8"自动检测"},
  {u8"zh",     u8"中文简体"},
  {u8"zh-CNS", u8"中文简体"},
  {u8"cht",    u8"中文繁体"},
  {u8"en",     u8"英语"},
  {u8"ja",     u8"日语"},
  {u8"jp",     u8"日语"},
  {u8"fra",    u8"法语"},
  {u8"fr",     u8"法语"},
  {u8"ko",     u8"韩语"},
  {u8"kor",    u8"韩语"},
  {u8"ru",     u8"俄语"},
};

const Translate::LanguageMap Translate::m_LanguageCanFromTo
{
  {  // baidu
    {u8"auto", {u8"zh", u8"cht", u8"en", u8"jp", u8"fra", u8"ru", u8"kor"}},
    {u8"zh",   {u8"cht", u8"en", u8"jp", u8"fra", u8"ru", u8"kor"}},
    {u8"cht",  {u8"zh", u8"en", u8"jp", u8"fra", u8"ru", u8"kor"}},
    {u8"en",   {u8"zh", u8"cht", u8"jp", u8"fra", u8"ru", u8"kor"}},
    {u8"jp",   {u8"zh", u8"cht", u8"en", u8"fra", u8"ru", u8"kor"}},
    {u8"fra",  {u8"zh", u8"cht", u8"en", u8"jp", u8"ru", u8"kor"}},
    {u8"ru",   {u8"zh", u8"cht", u8"en", u8"jp", u8"fra", u8"kor"}},
    {u8"kor",   {u8"zh", u8"cht", u8"en", u8"jp", u8"fra", u8"ru"}},
   },{  // youdao
    {u8"auto",   {u8"auto", u8"zh-CNS", u8"en", u8"ja", u8"ko", u8"fr", u8"ru"}},
    {u8"zh-CNS", {u8"auto", u8"en", u8"ja", u8"ko", u8"fr", u8"ru"}},
    {u8"en",     {u8"auto", u8"zh-CNS", u8"ja"}},
    {u8"ja",     {u8"auto", u8"zh-CNS", u8"en"}},
    {u8"ko",     {u8"auto", u8"zh-CNS"}},
    {u8"fr",     {u8"auto", u8"zh-CNS"}},
    {u8"ru",     {u8"auto", u8"zh-CNS"}},
  }
};

LanPair::LanPair(YJson& array)
  : from(array[0].getValueDouble())
  , to(array[1].getValueDouble())
{
}

Translate::Translate(YJson& setting)
    : m_LanPairBaidu(setting[u8"PairBaidu"])
    , m_LanPairYoudao(setting[u8"PairYoudao"])
    , m_LanPair(nullptr)
{
  SetSource(Baidu);
}

Translate::~Translate() {}

void Translate::SetSource(Source dict) {
  m_Source = dict;
  m_LanPair = (dict == Baidu) ? &m_LanPairBaidu: &m_LanPairYoudao;
}

inline static std::u8string GetSalt() {
  auto const now = chrono::system_clock::now();
  auto const count = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  std::string data = std::format("{}", count);
  return std::u8string(data.begin(), data.end());
}

std::u8string Translate::GetResultBaidu(const std::u8string& text) {
  // http://api.fanyi.baidu.com/product/113
  static YJson jsData {
    YJson::O{
      {u8"from", u8"auto"},
      {u8"to", u8"zh"},
      {u8"q", YJson::String},
      {u8"appid", u8"20210503000812254"},
    }
  };

  const auto& salt = GetSalt();
  jsData[u8"from"] = m_LanguageCanFromTo[Baidu][m_LanPair->f()].first;
  jsData[u8"to"] = m_LanguageCanFromTo[Baidu][m_LanPair->f()].second[m_LanPair->t()];
  jsData[u8"q"] = text;
  jsData[u8"salt"] = salt;
  jsData[u8"sign"].setText(Md5(u8"20210503000812254"s + text + salt + u8"Q_2PPxmCr66r6B2hi0ts"));

  HttpLib clt(jsData.urlEncode(u8"https://fanyi-api.baidu.com/api/trans/vip/translate?"));
  auto res = clt.Get();

  if (res->status == 200 || res->status == 201) {
    YJson jsData(res->body.begin(), res->body.end());
    if (auto iter = jsData.find(u8"error_msg"); iter != jsData.endO()) {
      // Access token expired
      return std::u8string(res->body.begin(), res->body.end());
      // return iter->second.getValueString();
    }
    const auto& obTransResult =
        jsData[u8"trans_result"].getArray();

    std::u8string content;
    for (auto& item : obTransResult) {
      content.append(item[u8"dst"].getValueString());
      content.push_back('\n');
    }
    return content;
  }
  return u8"<h1>没网络了！</h1>"s;
}

std::u8string Translate::GetResultYoudao(const std::u8string& text) {
  static const char8_t APP_KEY[]{u8"0b5f90d14623b917"};
  static const char8_t APP_SECRET[]{u8"8X1HcIvXXETCRf2smIbey8AGJ2xGRyK3"};
  static YJson m_scJson {
    YJson::O{
      {u8"from", u8"auto"},
      {u8"to", u8"auto"},
      {u8"appKey", APP_KEY},
      {u8"signType", u8"v3"},
      {u8"curtime", YJson::String},
      {u8"q", YJson::Null},
      {u8"salt", YJson::Null},
      {u8"sign", YJson::Null}
    }
  };

  m_scJson[u8"from"] = m_LanguageCanFromTo[Youdao][m_LanPair->f()].first;
  m_scJson[u8"to"] = m_LanguageCanFromTo[Youdao][m_LanPair->f()].second[m_LanPair->t()];

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
    FormatYoudaoResult(content, jsData);
    return content;
  }
  return content;
}

std::u8string Translate::GetResult(const std::u8string& text) {
  return m_Source == Youdao ? GetResultYoudao(text) : GetResultBaidu(text);
}

void Translate::FormatYoudaoResult(std::u8string& result, const YJson& data) {
  auto const& err = data[u8"errorCode"].getValueString();
  if (err != u8"0") {
    result = err;
    return;
  }
  std::ostringstream html;

  // 查询内容
  html << "<h3>" << data[u8"query"].getValueString();

  // basic 基本词典，查词时才有
  if (auto basic = data[u8"basic"]; basic.isObject()) {

    // 获取读音
    auto const& l = data[u8"l"].getValueString();

    static const std::u8string lst[][2] {
      {u8"英"s, u8"uk-phonetic"s},
      {u8"美"s, u8"us-phonetic"s},
      {u8"拼音"s, u8"phonetic"s},
      {u8"读音"s, u8"phonetic"s},
    };
    size_t left = 0, right = 0;
    if (l.starts_with(u8"en2")) {
      left = 0;
      right = 2;
    } else if (l.starts_with(u8"zh")) {
      left = 2;
      right = 3;
    } else {
      left = 3;
      right = 4;
    }

    for (int i = left; i != right; ++i) {
      auto const& phonetic = basic[lst[i][1]];
      if (!phonetic.isString()) continue;
      html << "  " << lst[i][0] << "[<span style='color:#44EEEE;'>" << phonetic.getValueString() << "</span>]";
    }

    html << "</h3><hr/>";

    // 获取
    if (auto wfs = basic[u8"wfs"]; wfs.isArray() && !wfs.emptyA()) {
      html << "<h5>形式变化</h5><ul>";
      for (auto& wf : wfs.getArray()) {
        auto temp = wf.beginO();
        if (temp == wf.endO() || temp->first != u8"wf")
          break;
        html << "<li style='color:#FF77CC;'>"
          << temp->second.find(u8"name")->second.getValueString()
          << ": "
          << temp->second.find(u8"value")->second.getValueString()
          << "</li>";
      }
      html << "</ul><hr/>";
    }

    // 	基本释义
    if (const auto& explains = basic[u8"explains"]; explains.isArray() && !explains.emptyA()) {
      html << "<h5>基本释义</h5><ul>";
      for (const auto& i : explains.getArray()) {
        html << "<li>" << i.getValueString() << "</li>";
      }
      html << "</ul><hr/>";
    }
  } else {
    html << "</h3><hr/>";
  }

  // 网络释义
  if (auto web = data[u8"web"]; web.isArray() && !web.emptyA()) {
    html << "<h5>网络释义</h5><ol>";
    for (const auto& i : web.getArray()) {
      html << "<li><span style='color: #FF00FF;'>"
        << i[u8"key"].getValueString()
        << "</span> &lt;";
      const auto& value = i[u8"value"].getArray();
      for (auto& j : value) {
        html << j.getValueString() << "; ";
      }
      html << "&gt;</li>";
    }
    html << "</ol><hr/>";
  }

  // 翻译结果
  if (auto const& translation = data[u8"translation"]; translation.isArray() && !translation.emptyA()) {
    html << "<h5>翻译结果</h5><li><ul>";
    for (auto const& i : translation.getArray()) {
      html << i.getValueString() << "; ";
    }
    html << "</ul></li><hr/>";
  }

  // html << "<br>" << l;
  auto const view = html.rdbuf()->view();
  result.assign(view.cbegin(), view.cend());
}
