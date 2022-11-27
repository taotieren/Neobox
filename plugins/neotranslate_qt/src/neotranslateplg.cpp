#include <neotranslateplg.h>
#include <translatedlg.h>
#include <yjson.h>

#include <QPlainTextEdit>

#define CLASS_NAME NeoTranslatePlg
#include <pluginexport.cpp>

/*
 * NeoTranslatePlugin
 */

NeoTranslatePlg::NeoTranslatePlg(YJson& settings):
  PluginObject(InitSettings(settings), u8"neotranslateplg", u8"极简翻译"),
  m_TranslateDlg(new NeoTranslateDlg(settings))
{
  AddMainObject(m_TranslateDlg);
  InitFunctionMap();
  m_Following.push_back({u8"neospeedboxplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::MouseMove) {
      if (m_TranslateDlg->isVisible())
        m_TranslateDlg->hide();
    } else if (event == PluginEvent::MouseDoubleClick) {
      if (m_TranslateDlg->isVisible())
        m_TranslateDlg->hide();
      else
        m_TranslateDlg->show();
    }
  }});
  m_Following.push_back({u8"neoocrplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::U8string) {
      if (!data) return;
      const auto& str = *reinterpret_cast<std::u8string*>(data);
      auto const txtfrom = m_TranslateDlg->findChild<QPlainTextEdit*>("neoPlainTextFrom");
      txtfrom->setPlainText(Utf82QString(str));
      m_TranslateDlg->show();
    }
  }});
}

NeoTranslatePlg::~NeoTranslatePlg()
{
  delete m_TranslateDlg;  // must delete the dlg when plugin destroying.
}

void NeoTranslatePlg::InitFunctionMap() {
  m_FunctionMapVoid = {
    {u8"toggleVisibility",
      {u8"打开窗口", u8"打开或关闭极简翻译界面", std::bind(&NeoTranslateDlg::ToggleVisibility, m_TranslateDlg)}
    }
  };
  m_FunctionMapBool = {
    {u8"enableReadClipboard",
      {u8"读剪切板", u8"打开界面自动读取剪切板内容到From区", [this](bool checked) {
        m_Settings[u8"AutoTranslate"] = checked;
        mgr->SaveSettings();
      }, std::bind(&YJson::isTrue, &m_Settings[u8"AutoTranslate"])}
    },
    {u8"enableAutoTranslate",
      {u8"自动翻译", u8"打开界面自动翻译From区内容", [this](bool checked) {
        m_Settings[u8"ReadClipboard"] = checked;
        mgr->SaveSettings();
      }, std::bind(&YJson::isTrue, &m_Settings[u8"ReadClipboard"])}
    },
  };
}

void NeoTranslatePlg::InitMenuAction(QMenu* pluginMenu)
{
  PluginObject::InitMenuAction(pluginMenu);
}

YJson& NeoTranslatePlg::InitSettings(YJson& settings)
{
  if (settings.isObject()) return settings;
  return settings = YJson::O {
    { u8"Mode", 0 },
    { u8"AutoTranslate", false },
    { u8"ReadClipboard", false },
  };
  // we may not need to call SaveSettings;
}