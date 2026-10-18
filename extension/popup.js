const statusEl = document.getElementById('status');
const toggleBtn = document.getElementById('toggleBtn');

// Проверяем текущее состояние при открытии всплывающего окна
chrome.storage.local.get(['tpwsEnabled'], (result) => {
  const isEnabled = !!result.tpwsEnabled;
  updateUI(isEnabled);
});

// Обработчик клика по кнопке переключения
toggleBtn.addEventListener('click', () => {
  chrome.storage.local.get(['tpwsEnabled'], (result) => {
    const newState = !result.tpwsEnabled;
    if (newState) {
      enableProxy();
    } else {
      disableProxy();
    }
  });
});

// Обновление интерфейса
function updateUI(isEnabled) {
  if (isEnabled) {
    statusEl.textContent = 'Работает (127.0.0.1:1080)';
    statusEl.className = 'status active';
    toggleBtn.textContent = 'Отключить';
    toggleBtn.className = 'btn-disable';
  } else {
    statusEl.textContent = 'Отключено (Напрямую)';
    statusEl.className = 'status';
    toggleBtn.textContent = 'Включить';
    toggleBtn.className = 'btn-enable';
  }
}

// Включение SOCKS5 прокси
function enableProxy() {
  const config = {
    mode: "fixed_servers",
    rules: {
      singleProxy: {
        scheme: "socks5",
        host: "127.0.0.1",
        port: 1080
      }
    }
  };

  chrome.proxy.settings.set({ value: config, scope: 'regular' }, () => {
    chrome.storage.local.set({ tpwsEnabled: true }, () => {
      updateUI(true);
    });
  });
}

// Отключение прокси (возврат к прямому соединению)
function disableProxy() {
  const config = {
    mode: "direct"
  };

  chrome.proxy.settings.set({ value: config, scope: 'regular' }, () => {
    chrome.storage.local.set({ tpwsEnabled: false }, () => {
      updateUI(false);
    });
  });
}