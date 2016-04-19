local pcall, dofile, _G = pcall, dofile, _G

module "luci.version"

if pcall(dofile, "/etc/openwrt_release") and _G.DISTRIB_DESCRIPTION then
	distname    = ""
	distversion = _G.DISTRIB_DESCRIPTION
else
	distname    = "OpenWrt Firmware"
	distversion = "Barrier Breaker (r45620)"
end

luciname    = "LuCI 0.12 Branch"
luciversion = "0.12+git-15.112.69076-6153b99"
