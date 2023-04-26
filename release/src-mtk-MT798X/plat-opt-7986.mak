define platformL2Router
	@( \
		if [ "$(MT7986A)" = "y" ]; then \
			sed -i "/RTCONFIG_SOC_MT7986A\>/d" $(1); \
			echo "RTCONFIG_SOC_MT7986A=y" >>$(1); \
		elif [ "$(MT7986B)" = "y" ]; then \
			sed -i "/RTCONFIG_SOC_MT7986B\>/d" $(1); \
			echo "RTCONFIG_SOC_MT7986B=y" >>$(1); \
		elif [ "$(MT7986C)" = "y" ]; then \
			sed -i "/RTCONFIG_SOC_MT7986C\>/d" $(1); \
			echo "RTCONFIG_SOC_MT7986C=y" >>$(1); \
		fi; \
		if [ "$(MT7531)" = "y" ]; then \
			sed -i "/RTCONFIG_SWITCH_MT7986_MT7531\>/d" $(1); \
			echo "RTCONFIG_SWITCH_MT7986_MT7531=y" >>$(1); \
		fi; \
		if [ "$(BUILD_NAME)" = "TUF-AX6000" ]; then \
			sed -i "/RTCONFIG_PWMX2_GPIOX1_RGBLED\>/d" $(1); \
			echo "RTCONFIG_PWMX2_GPIOX1_RGBLED=y" >>$(1); \
		elif [ "$(BUILD_NAME)" = "RT-AX59U" ] ; then \
			sed -i "/RTCONFIG_PWM_RGBLED\>/d" $(1); \
			echo "RTCONFIG_PWM_RGBLED=y" >>$(1); \
		fi; \
	)
endef

define platformL2Busybox
	@( true )
endef

define platformL2Kernel
	@( \
	if [ "$(BUILD_NAME)" = "TUF-AX4200" ]; then \
		sed -i "/CONFIG_OVERLAY_FS\>/d" $(1); \
		echo "CONFIG_OVERLAY_FS=y" >>$(1); \
		sed -i "/CONFIG_OVERLAY_FS_REDIRECT_DIR\>/d" $(1); \
		echo "# CONFIG_OVERLAY_FS_REDIRECT_DIR is not set" >>$(1); \
		sed -i "/CONFIG_OVERLAY_FS_REDIRECT_ALWAYS_FOLLOW\>/d" $(1); \
		echo "# CONFIG_OVERLAY_FS_REDIRECT_ALWAYS_FOLLOW is not set" >>$(1); \
		sed -i "/CONFIG_OVERLAY_FS_INDEX\>/d" $(1); \
		echo "# CONFIG_OVERLAY_FS_INDEX is not set" >>$(1); \
		sed -i "/CONFIG_OVERLAY_FS_XINO_AUTO\>/d" $(1); \
		echo "# CONFIG_OVERLAY_FS_XINO_AUTO is not set" >>$(1); \
		sed -i "/CONFIG_OVERLAY_FS_METACOPY\>/d" $(1); \
		echo "# CONFIG_OVERLAY_FS_METACOPY is not set" >>$(1); \
	fi; \
	)
endef

