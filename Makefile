#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=monic
PKG_VERSION:=0.1.0
PKG_RELEASE:=1
STRIP:=1
SOURCE_DIR:=$(TOPDIR)/dotin/src/monic
PKG_BUILD_DIR:=$(BUILD_DIR)/monic-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/monic
	CATEGORY:=Development
	TITLE:=Embedded Monitor Solution
	DEPENDS:=+libstdcpp
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)/
endef



define Package/monic/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/usr/share/monic $(1)/etc/init.d 
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/monic $(1)/usr/bin/
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/monic.service $(1)/etc/init.d/monic
endef

$(eval $(call BuildPackage,monic))


