
#include "ui-obs-browser-widget.hpp"
#include "plugin.hpp"

#include "warning-disable.hpp"
#include <mutex>

#include <../plugins/obs-browser/panel/browser-panel.hpp>
#ifdef D_PLATFORM_LINUX
#include <obs-nix-platform.h>
#endif
#include "warning-enable.hpp"

streamfx::ui::obs_browser_cef::obs_browser_cef()
{
#ifdef D_PLATFORM_LINUX
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND) {
		throw std::runtime_error("Wayland does not support Browser Widgets.");
	}
#endif

	// Load the "obs-browser" module.
	_module = util::library::load(obs_get_module("obs-browser"));
	auto fn = reinterpret_cast<QCef* (*)(void)>(_module->load_symbol("obs_browser_create_qcef"));
	if (!fn) {
		throw std::runtime_error("Failed to load obs-browser module.");
	}

	// Create a QCef instance and initialize it.
	_cef = fn();
	if (!_cef) {
		throw std::runtime_error("Failed to create or get QCef instance.");
	}
	reinterpret_cast<QCef*>(_cef)->init_browser();
	reinterpret_cast<QCef*>(_cef)->wait_for_browser_init();

	// Create a generic Cookie manager for widgets.
	_cookie =
		reinterpret_cast<QCef*>(_cef)->create_cookie_manager(streamfx::config_file_path("cookies").u8string(), false);
}

streamfx::ui::obs_browser_cef::~obs_browser_cef()
{
	delete reinterpret_cast<QCefCookieManager*>(_cookie);
	delete reinterpret_cast<QCef*>(_cef);
}

void* streamfx::ui::obs_browser_cef::cef()
{
	return _cef;
}

void* streamfx::ui::obs_browser_cef::cookie_manager()
{
	return _cookie;
}

std::shared_ptr<streamfx::ui::obs_browser_cef> streamfx::ui::obs_browser_cef::instance()
{
	static std::weak_ptr<obs_browser_cef> ptr;
	static std::mutex                     lock;

	std::lock_guard<decltype(lock)> lg(lock);
	if (!ptr.expired()) {
		return ptr.lock();
	}

	std::shared_ptr<obs_browser_cef> sintance{new obs_browser_cef()};
	ptr = sintance;
	return sintance;
}

streamfx::ui::obs_browser_widget::obs_browser_widget(QUrl url, QWidget* parent) : QWidget(parent)
{
	_cef    = obs_browser_cef::instance();
	_widget = reinterpret_cast<QCef*>(_cef->cef())
				  ->create_widget(this, url.toString().toStdString(),
								  reinterpret_cast<QCefCookieManager*>(_cef->cookie_manager()));
	if (!_widget) {
		throw std::runtime_error("Failed to create QCefWidget.");
	}

	// Add a proper layout.
	_layout = new QHBoxLayout();
	_layout->setContentsMargins(0, 0, 0, 0);
	_layout->setSpacing(0);
	this->setLayout(_layout);
	_layout->addWidget(_widget);

	// Disable all popups.
	dynamic_cast<QCefWidget*>(_widget)->allowAllPopups(false);
}

streamfx::ui::obs_browser_widget::~obs_browser_widget() {}

void streamfx::ui::obs_browser_widget::set_url(QUrl url)
{
	dynamic_cast<QCefWidget*>(_widget)->setURL(url.toString().toStdString());
}

bool streamfx::ui::obs_browser_widget::is_available()
{
#ifdef D_PLATFORM_LINUX
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND) {
		return false;
	}
#endif
	return true;
}
