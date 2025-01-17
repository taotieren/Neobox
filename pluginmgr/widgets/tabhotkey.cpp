#include "tabhotkey.hpp"
#include "plugincenter.hpp"

#include <pluginmgr.h>
#include <pluginobject.h>
#include <yjson.h>
#include <ui_tabhotkey.h>
#include <shortcut.h>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileDialog>

using namespace std::literals;

HotKeyInfoPlugin::HotKeyInfoPlugin(const YJson& data, bool on)
  : data(data)
  , pluginName(this->data[u8"PluginName"].getValueString())
  , function(this->data[u8"Function"].getValueString())
  , enabled(on)
{}

HotKeyInfoCommand::HotKeyInfoCommand(const YJson& data, bool on)
  : data(data)
  , executable(this->data[u8"Executable"].getValueString())
  , directory(this->data[u8"Directory"].getValueString())
  , arguments(this->data[u8"Arguments"].getValueString())
  , enabled(on)
{}

TabHotKey::TabHotKey(PluginCenter* center)
  : QWidget(center)
  , m_Settings(mgr->GetEventMap())
  , m_Shortcut(*mgr->m_Shortcut)
  , ui(new Ui::FormHotKey)
{
  InitLayout();
  InitDataStruct();
  InitPluginCombox();
  InitSignals();
  if (ui->cBoxPlugin->count()) {
    UpdatePluginMethord(0);
  }
  if (ui->listWidget->count()) {
    ui->listWidget->setCurrentRow(0);
  }
}

TabHotKey::~TabHotKey()
{
  delete ui;
}

void TabHotKey::InitLayout()
{
  auto const background = new QWidget(this);
  auto mainLayout  = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0,0,0,0);
  mainLayout->addWidget(background);
  ui->setupUi(background);
  background->setObjectName("whiteBackground");
}

void TabHotKey::InitSignals()
{
  connect(ui->tBtnDirectory, &QToolButton::clicked, this, &TabHotKey::ChooseDirectory);
  connect(ui->tBtnProgram, &QToolButton::clicked, this, &TabHotKey::ChooseProgram);
  connect(ui->listWidget, &QListWidget::currentTextChanged, this, &TabHotKey::UpdateHotKeyEditor, Qt::DirectConnection);
  connect(ui->pBtnHotKey, &QPushButton::clicked, this, &TabHotKey::ChangeEnabled);
  connect(ui->pBtnSaveContent, &QPushButton::clicked, this, &TabHotKey::SaveHotKeyData);
  connect(ui->cBoxPlugin, &QComboBox::currentIndexChanged, this, &TabHotKey::UpdatePluginMethord, Qt::DirectConnection);
  connect(ui->pBtnAdd, &QPushButton::clicked, this, &TabHotKey::AddEmptyItem);
  connect(ui->pBtnRemove, &QPushButton::clicked, this, &TabHotKey::RemoveItem);
  connect(ui->rBtnPlugin, &QRadioButton::toggled, this, [this](bool on) {
    if (on) {
      ui->gBoxPlugin->setEnabled(true);
      ui->gBoxProcess->setEnabled(false);
    } else {
      ui->gBoxPlugin->setEnabled(false);
      ui->gBoxProcess->setEnabled(true);
    }
  });

  ui->pBtnHotKey->installEventFilter(this);
}

void TabHotKey::InitDataStruct()
{
  for (const auto& info: m_Settings.getArray()) {
    auto keySequence = PluginObject::Utf82QString(info[u8"KeySequence"].getValueString());
    bool const enabled = info[u8"Enabled"].isTrue();
    ui->listWidget->addItem(keySequence);
    if (auto iter = info.find(u8"Plugin"); iter != info.endO()) {
      m_Plugins[keySequence] = std::make_unique<HotKeyInfoPlugin>(iter->second, enabled);
    } else if (auto iter = info.find(u8"Command"); iter != info.endO()) {
      m_Commands[keySequence] = std::make_unique<HotKeyInfoCommand>(iter->second, enabled);
    }
  }
}

void TabHotKey::InitPluginCombox()
{
  QStandardItemModel *model = new QStandardItemModel;
  for (auto& [name, info]: mgr->GetPluginsInfo().getObject()) {
    auto const item = new QStandardItem(PluginObject::Utf82QString(info[u8"FriendlyName"].getValueString()));
    item->setToolTip(PluginObject::Utf82QString(name));
    m_PluginNames.push_back(name);
    model->appendRow(item);
  }
  ui->cBoxPlugin->setModel(model);
}

void TabHotKey::UpdateHotKeyEditor(QString text)
{
  ui->pBtnHotKey->setText(text);

  bool a = true, b = true;
  if (auto iter = m_Plugins.find(text); iter != m_Plugins.end()) {
    const auto& info = *iter->second;
    ui->rBtnPlugin->setChecked(true);
    ui->chkRegisterHotKey->setChecked(info.enabled);
    ui->cBoxPlugin->setCurrentText(PluginObject::Utf82QString(mgr->GetPluginsInfo()[info.pluginName][u8"FriendlyName"].getValueString()));
    ui->cBoxCallBack->setCurrentText(PluginObject::Utf82QString(info.function));
    b = false;
  } else if (auto iter = m_Commands.find(text); iter != m_Commands.end()) {
    const auto& info = *iter->second;
    ui->rBtnProcess->setChecked(true);
    ui->chkRegisterHotKey->setChecked(info.enabled);
    ui->lineProgram->setText(PluginObject::Utf82QString(info.executable));
    ui->lineArguments->setText(PluginObject::Utf82QString(info.arguments));
    ui->lineDirectory->setText(PluginObject::Utf82QString(info.directory));
    a = false;
  }
  if (a && b) {
    ui->chkRegisterHotKey->setChecked(false);
  }
  if (a) {
    ui->lineProgram->setText("shutdown.exe");
    ui->lineArguments->setText("-s -t 60");
    ui->lineDirectory->setText(".");
  }
  if (b) {
    if (ui->cBoxPlugin->count())
      ui->cBoxPlugin->setCurrentIndex(0);
    if (ui->cBoxCallBack->count())
      ui->cBoxCallBack->setCurrentIndex(0);
  }
}

void TabHotKey::UpdatePluginMethord(int index)
{
  ui->cBoxCallBack->clear();
  if (index < 0 || index >= m_PluginNames.size()) {
    return;
  }
  auto& pluginName = m_PluginNames[index];
  auto iter = mgr->m_Plugins.find(pluginName);
  if (iter == mgr->m_Plugins.end()) {
    return;
  }

  QStandardItemModel *model = new QStandardItemModel;
  auto& methords = iter->second.plugin->m_PluginMethod;
  for (auto& [name, info]: methords) {
    if (info.type != PluginEvent::Void) continue;
    auto const item = new QStandardItem(PluginObject::Utf82QString(name));
    item->setToolTip(PluginObject::Utf82QString(u8"<" + info.friendlyName + u8">: " + info.description));
    model->appendRow(item);
  }
  ui->cBoxCallBack->setModel(model);
}

void TabHotKey::ChangeEnabled(bool on)
{
  static QString keys;
  if (on) {
    keys = ui->pBtnHotKey->text();
  } else {
    auto text = ui->pBtnHotKey->text();
    if (text != keys) {
      if (m_Commands.find(text) != m_Commands.end() || m_Plugins.find(text) != m_Plugins.end()) {
        ui->pBtnHotKey->setText(keys);
        QMessageBox::information(this, "错误", "不能录制一个已经存在的热键！");
      } else {
        keys = text;
      }
    }
  }

  on = !on;

  ui->listWidget->setEnabled(on);
  ui->gBoxChooseType->setEnabled(on);

  ui->gBoxPlugin->setEnabled(on && ui->rBtnPlugin->isChecked());
  ui->gBoxProcess->setEnabled(on && ui->rBtnProcess->isChecked());

  ui->chkRegisterHotKey->setEnabled(on);
  ui->pBtnSaveContent->setEnabled(on);

  ui->pBtnAdd->setEnabled(on);
  ui->pBtnRemove->setEnabled(on);

  std::initializer_list<QWidget*> lst = {
    ui->chkRegisterHotKey, ui->pBtnSaveContent, ui->pBtnAdd, ui->pBtnRemove
  };
  for (auto i: lst) {
    style()->unpolish(i);
    style()->polish(i);
  }
}

void TabHotKey::ChooseDirectory()
{
  auto curDir = ui->lineDirectory->text();
  if (!QDir().exists(curDir)) curDir = ".";
  auto dirString = QFileDialog::getExistingDirectory(this, "选择程序工作目录", curDir);
  if (dirString.isEmpty()) return;
  ui->lineDirectory->setText(QDir::toNativeSeparators(dirString));
}

void TabHotKey::ChooseProgram()
{
  auto curDir = ui->lineProgram->text();
  if (!QFile::exists(curDir)) {
    curDir = ".";
  } else {
    curDir.push_back("/..");
  }
  auto fileString = QFileDialog::getOpenFileName(this, "选择程序", curDir);
  if (fileString.isEmpty()) return;
  ui->lineProgram->setText(QDir::toNativeSeparators(fileString));
}

bool TabHotKey::SaveHotKeyData()
{
  if (!ui->listWidget->currentItem()) {
    QMessageBox::information(this, "提示", "请先点击左下方加号，添加一个新的配置文件！");
    return false;
  }
  if (!IsDataInvalid()) {
    QMessageBox::information(this, "错误", "数据冲突或数据不完整，无法保存。");
    return false;
  }

  bool needSave = true;
  YJson* ptrEnabled = nullptr;
  const auto currKey = ui->pBtnHotKey->text();

  auto item = ui->listWidget->currentItem();
  const auto prevKey = item->text();

  auto const enabled = ui->chkRegisterHotKey->isChecked();
  auto const u8PrevKey = PluginObject::QString2Utf8(prevKey);
  if (prevKey != currKey) {
    // 删除之前的数据
    auto iter = std::find_if(m_Settings.beginA(), m_Settings.endA(), [&u8PrevKey](const YJson& info){
      return info[u8"KeySequence"].getValueString() == u8PrevKey;
    });
    if (iter != m_Settings.endA()) {
      // unregister hot key
      if (iter->find(u8"Enabled")->second.isTrue()) {
        m_Shortcut.UnregisterHotKey(u8PrevKey);
      }

      m_Commands.erase(prevKey);
      m_Plugins.erase(prevKey);

      m_Settings.remove(iter);
    }

    // 保存现在的数据
    auto u8CurrKey = PluginObject::QString2Utf8(currKey);
    if (ui->rBtnProcess->isChecked()) {
      auto iter = m_Settings.append(YJson {
        YJson::O {
          {u8"KeySequence", u8CurrKey},
          {u8"Enabled", enabled},
          {u8"Command", YJson::O {
            {u8"Executable", PluginObject::QString2Utf8(ui->lineProgram->text())},
            {u8"Directory", PluginObject::QString2Utf8(ui->lineDirectory->text())},
            {u8"Arguments", PluginObject::QString2Utf8(ui->lineArguments->text())}
          }},
        }
      });
      m_Commands[currKey] = std::make_unique<HotKeyInfoCommand>(
        iter->find(u8"Command")->second,
        enabled
      );
      ptrEnabled = &iter->find(u8"Enabled")->second;
    } else {
      auto iter = m_Settings.append(YJson {
        YJson::O {
          {u8"KeySequence", u8CurrKey},
          {u8"Enabled", enabled},
          {u8"Plugin", YJson::O {
            {u8"PluginName", m_PluginNames[ui->cBoxPlugin->currentIndex()]},
            {u8"Function", PluginObject::QString2Utf8(ui->cBoxCallBack->currentText())},
          }},
        }
      });
      m_Plugins[currKey] = std::make_unique<HotKeyInfoPlugin>(
        iter->find(u8"Plugin")->second, enabled
      );
      ptrEnabled = &iter->find(u8"Enabled")->second;
    }
    item->setText(currKey);
    if (ptrEnabled->isTrue()) {
      *ptrEnabled = m_Shortcut.RegisterHotKey(u8CurrKey);
      if (!ptrEnabled->isTrue()) {
        ui->chkRegisterHotKey->setChecked(false);
      }
    }
  } else {
    bool prevRegisterKey = false;
    auto& jsData = *std::find_if(m_Settings.beginA(), m_Settings.endA(), [&u8PrevKey](const YJson& info){
      return info[u8"KeySequence"] == u8PrevKey;
    });
    ptrEnabled = &jsData.find(u8"Enabled")->second;

    if (auto iter = m_Plugins.find(prevKey); iter != m_Plugins.end()) {
      auto& info =  *iter->second;
      prevRegisterKey = info.enabled;
      auto jsIter = jsData.find(u8"Plugin");

      if (ui->rBtnProcess->isChecked()) {
        m_Plugins.erase(iter);
        jsIter->first = u8"Command";
        jsIter->second = YJson::O {
          {u8"Executable", PluginObject::QString2Utf8(ui->lineProgram->text())},
          {u8"Directory", PluginObject::QString2Utf8(ui->lineDirectory->text())},
          {u8"Arguments", PluginObject::QString2Utf8(ui->lineArguments->text())}
        };
        m_Commands[currKey] = std::make_unique<HotKeyInfoCommand>(
          jsIter->second, ui->chkRegisterHotKey->isChecked()
        );
      } else {
        const auto& pluginName = m_PluginNames[ui->cBoxPlugin->currentIndex()];
        auto function = PluginObject::QString2Utf8(ui->cBoxCallBack->currentText());

        if (pluginName != info.pluginName || function != info.function) {
          info.pluginName = std::move(pluginName);
          info.function = std::move(function);
          jsIter->second = info.GetJson();
        } else {
          needSave = false;
        }
      }
    } else if (auto iter = m_Commands.find(prevKey); iter != m_Commands.end()) {
      auto& info =  *iter->second;
      prevRegisterKey = info.enabled;
      auto jsIter = jsData.find(u8"Command");

      if (ui->rBtnProcess->isChecked()) {
        auto executable = PluginObject::QString2Utf8(ui->lineProgram->text());
        auto directory = PluginObject::QString2Utf8(ui->lineDirectory->text());
        auto arguments = PluginObject::QString2Utf8(ui->lineArguments->text());

        if (executable != info.executable || directory != info.directory || arguments != info.arguments) {
          info.executable = std::move(executable);
          info.directory = std::move(directory);
          info.arguments = std::move(arguments);
          jsIter->second = info.GetJson();
        } else {
          needSave = false;
        }
      } else {
        m_Commands.erase(iter);
        jsIter->first = u8"Plugin";

        jsIter->second = YJson::O {
          {u8"PluginName", m_PluginNames[ui->cBoxPlugin->currentIndex()]},
          {u8"Function", PluginObject::QString2Utf8(ui->cBoxCallBack->currentText())},
        };
        m_Plugins[currKey] = std::make_unique<HotKeyInfoPlugin>(
          jsIter->second, enabled
        );
      }
    }
    if (prevRegisterKey != enabled) {
      if (ui->chkRegisterHotKey->isChecked()) {
        *ptrEnabled = m_Shortcut.RegisterHotKey(u8PrevKey);
        if (!ptrEnabled->isTrue()) {
          ui->chkRegisterHotKey->setChecked(false);
        }
      } else {
        m_Shortcut.UnregisterHotKey(u8PrevKey);
      }
      needSave = true;
    }
  }
  if (needSave) {
    mgr->SaveSettings();
    mgr->ShowMsg("保存成功！");
  } else {
    // mgr->ShowMsg("无需保存！");
  }
  return true;
}

bool TabHotKey::IsDataInvalid() const
{
  if (!ui->listWidget->currentItem()) {
    return false;
  }
  bool result = true;
  if (ui->rBtnProcess->isChecked()) {
    auto text = ui->lineProgram->text();
    result = result && !text.isEmpty();
    text = ui->lineArguments->text();
    result = result && !text.isEmpty();
    text = ui->lineDirectory->text();
    result = result && !text.isEmpty();
  } else {
    result = result && ui->cBoxPlugin->currentIndex() != -1 && ui->cBoxCallBack->currentIndex() != -1;
  }
  result = result && !ui->pBtnHotKey->text().isEmpty();
  return result;
}

bool TabHotKey::eventFilter(QObject *target, QEvent *event)
{
  // https://blog.csdn.net/sunflover454/article/details/50904815
  if (target == ui->pBtnHotKey) {
    if(event->type() == QEvent::KeyPress && ui->pBtnHotKey->isChecked())
    {
      auto keyevent = static_cast<QKeyEvent*>(event);
      int uKey = keyevent->key();
      Qt::Key key = static_cast<Qt::Key>(uKey);
      if(key == Qt::Key_unknown)
      {
        //nothing { unknown key }
      }

      if(key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt )
      {
        return false;
      }
      auto modifiers = keyevent->modifiers();
      if(modifiers & Qt::ShiftModifier)
        uKey += Qt::SHIFT;
      if(modifiers & Qt::ControlModifier)
        uKey += Qt::CTRL;
      if(modifiers & Qt::AltModifier)
        uKey += Qt::ALT;
      ui->pBtnHotKey->setText(QKeySequence(uKey).toString(QKeySequence::NativeText));
      return true;
    }
  }

  return QWidget::eventFilter(target, event);
}

void TabHotKey::AddEmptyItem()
{
  ui->listWidget->addItem("");
  ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
}

void TabHotKey::RemoveItem()
{
  auto const index = ui->listWidget->currentRow();
  if (index == -1) return;

  auto const item = ui->listWidget->takeItem(index);
  auto const text = item->text();
  delete item;

  m_Plugins.erase(text);
  m_Commands.erase(text);
  
  if (text.isEmpty()) return;

  auto keyString = PluginObject::QString2Utf8(text);
  auto iter = std::find_if(m_Settings.beginA(), m_Settings.endA(), [&keyString](const YJson& data){
    return data[u8"KeySequence"].getValueString() == keyString;
  });

  if (iter == m_Settings.endA()) return;

  if (iter->find(u8"Enabled")->second.isTrue()) {
    m_Shortcut.UnregisterHotKey(keyString);
  }

  m_Settings.remove(iter);

  mgr->SaveSettings();
}
