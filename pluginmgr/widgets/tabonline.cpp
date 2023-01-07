#include "tabonline.hpp"
#include "plugincenter.hpp"
#include "itemonline.hpp"

#include <glbobject.h>
#include <pluginobject.h>
#include <pluginmgr.h>
#include <yjson.h>

#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

TabOnline::TabOnline(PluginCenter* center)
  : QWidget(center)
  , m_PluginCenter(*center)
  , m_MainLayout(new QVBoxLayout(this))
  // , m_ControlLayout(new QHBoxLayout)
  , m_ListWidget(new QListWidget(this))
{
  InitLayout();
  // InitControls();
  InitPlugins();
}

TabOnline::~TabOnline()
{
}

void TabOnline::UpdateItem(std::u8string_view pluginName, bool isUpdate)
{
  for (int i=0; i!=m_ListWidget->count(); ++i) {
    auto w = qobject_cast<ItemOnline*>(m_ListWidget->itemWidget(m_ListWidget->item(i)));
    if (w->m_PluginName == pluginName) {
      if (isUpdate) {
        w->SetUpdated();
      } else {
        w->SetUninstalled();
      }
      break;
    }
  }
}

void TabOnline::UpdatePlugins()
{
  if (!m_PluginCenter.m_PluginData && !m_PluginCenter.UpdatePluginData()) {
    glb->glbShowMsg("下载插件信息失败！");
    return;
  }

  InitPlugins();
}

void TabOnline::InitPlugins()
{
  if (!m_PluginCenter.m_PluginData)
    return;
  
  for (const auto& [name, info]: m_PluginCenter.m_PluginData->find(u8"Plugins")->second.getObject()) {
    auto const item = new QListWidgetItem;
    item->setSizeHint(QSize(400, 70));
    m_ListWidget->addItem(item);
    m_ListWidget->setItemWidget(item, new ItemOnline(name, info, m_ListWidget));
  }
  // m_UpdateButton->setEnabled(false);
}

// void TabOnline::InitControls()
// {
//   m_UpdateButton = new QPushButton("刷新", this);
//   m_ControlLayout->addWidget(m_UpdateButton);
//   connect(m_UpdateButton, &QPushButton::clicked, this, &TabOnline::UpdatePlugins);
// }

void TabOnline::InitLayout()
{
  m_MainLayout->addWidget(m_ListWidget);
  // m_MainLayout->addLayout(m_ControlLayout);
}