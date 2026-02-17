#ifndef WEBPAGE_NETWORK_SETTINGS_H
#define WEBPAGE_NETWORK_SETTINGS_H

/**
 * @file webpage_network_settings.h
 * @brief Network settings page renderer for WiFi/Ethernet configuration
 */

#include <Arduino.h>
#include "../config.h"
#include "config_manager.h"

namespace WebPageNetworkSettings {

inline void render(Print& out, const ConfigManager& configMgr) {
    const NetworkConfig& cfg = configMgr.getNetworkConfig();

    // HTTP Response
    out.println("HTTP/1.1 200 OK");
    out.println("Content-Type: text/html");
    out.println("Connection: close");
    out.println();

    // HTML
    out.println("<!DOCTYPE html><html lang='en'><head>");
    out.println("<meta charset='UTF-8'>");
    out.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    out.println("<title>Network Settings - PELNI GPS Tracker</title>");
    out.println("<style>");

    // CSS
    out.println("*{margin:0;padding:0;box-sizing:border-box}");
    out.println("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;padding:20px}");
    out.println(".container{max-width:500px;margin:0 auto}");
    out.println(".header{text-align:center;margin-bottom:30px}");
    out.println(".header h1{font-size:1.5rem;font-weight:600;color:#38bdf8}");
    out.println(".header p{font-size:.875rem;color:#64748b;margin-top:4px}");
    out.println(".card{background:#1e293b;border-radius:12px;padding:24px;border:1px solid #334155;margin-bottom:16px}");
    out.println(".card-title{font-size:1rem;font-weight:600;color:#f1f5f9;margin-bottom:20px;display:flex;align-items:center;gap:8px}");
    out.println(".card-title svg{width:20px;height:20px;stroke:#38bdf8}");
    out.println(".form-group{margin-bottom:16px}");
    out.println(".form-label{display:block;font-size:.75rem;color:#94a3b8;margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}");
    out.println(".form-input{width:100%;padding:12px;background:#0f172a;border:1px solid #334155;border-radius:8px;color:#e2e8f0;font-size:.875rem;outline:none;transition:border-color .2s}");
    out.println(".form-input:focus{border-color:#38bdf8}");
    out.println(".form-input::placeholder{color:#475569}");
    out.println(".form-input:disabled{opacity:.5;cursor:not-allowed}");
    out.println(".btn{padding:12px 24px;border:none;border-radius:8px;font-size:.875rem;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;justify-content:center;gap:8px}");
    out.println(".btn-primary{background:#38bdf8;color:#0f172a}.btn-primary:hover{background:#0ea5e9}");
    out.println(".btn-secondary{background:#334155;color:#e2e8f0}.btn-secondary:hover{background:#475569}");
    out.println(".btn-danger{background:#ef4444;color:#fff}.btn-danger:hover{background:#dc2626}");
    out.println(".btn-group{display:flex;gap:12px;margin-top:20px}");
    out.println(".btn svg{width:16px;height:16px}");
    out.println(".btn:disabled{opacity:.5;cursor:not-allowed}");
    out.println(".alert{padding:12px 16px;border-radius:8px;margin-bottom:16px;font-size:.875rem;display:none}");
    out.println(".alert.success{background:rgba(34,197,94,.2);border:1px solid rgba(34,197,94,.3);color:#4ade80}");
    out.println(".alert.error{background:rgba(239,68,68,.2);border:1px solid rgba(239,68,68,.3);color:#f87171}");
    out.println(".alert.warning{background:rgba(234,179,8,.2);border:1px solid rgba(234,179,8,.3);color:#facc15}");
    out.println(".alert.show{display:block}");
    out.println(".back-link{display:inline-flex;align-items:center;gap:6px;color:#64748b;text-decoration:none;font-size:.875rem;margin-bottom:20px}");
    out.println(".back-link:hover{color:#94a3b8}");
    out.println(".back-link svg{width:16px;height:16px}");
    out.println(".loading{opacity:.6;pointer-events:none}");

    // Mode selector
    out.println(".mode-selector{display:flex;gap:8px;margin-bottom:20px}");
    out.println(".mode-btn{flex:1;padding:16px;background:#0f172a;border:2px solid #334155;border-radius:10px;cursor:pointer;text-align:center;transition:all .2s}");
    out.println(".mode-btn:hover{border-color:#475569}");
    out.println(".mode-btn.active{border-color:#38bdf8;background:rgba(56,189,248,.1)}");
    out.println(".mode-btn svg{width:32px;height:32px;stroke:#64748b;margin-bottom:8px}");
    out.println(".mode-btn.active svg{stroke:#38bdf8}");
    out.println(".mode-btn-label{font-size:.875rem;font-weight:600;color:#94a3b8}");
    out.println(".mode-btn.active .mode-btn-label{color:#38bdf8}");

    // WiFi list
    out.println(".wifi-list{max-height:200px;overflow-y:auto;margin-bottom:16px;border:1px solid #334155;border-radius:8px}");
    out.println(".wifi-item{display:flex;align-items:center;justify-content:space-between;padding:12px;border-bottom:1px solid #334155;cursor:pointer;transition:background .2s}");
    out.println(".wifi-item:last-child{border-bottom:none}");
    out.println(".wifi-item:hover{background:#334155}");
    out.println(".wifi-item.selected{background:rgba(56,189,248,.1)}");
    out.println(".wifi-ssid{font-size:.875rem;color:#e2e8f0}");
    out.println(".wifi-signal{display:flex;align-items:center;gap:4px;font-size:.75rem;color:#64748b}");
    out.println(".wifi-signal svg{width:16px;height:16px}");
    out.println(".wifi-loading{text-align:center;padding:20px;color:#64748b;font-size:.875rem}");
    out.println(".wifi-empty{text-align:center;padding:20px;color:#64748b;font-size:.875rem}");

    // Password visibility toggle
    out.println(".password-wrapper{position:relative}");
    out.println(".password-toggle{position:absolute;right:12px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;color:#64748b;padding:4px}");
    out.println(".password-toggle:hover{color:#94a3b8}");
    out.println(".password-toggle svg{width:20px;height:20px}");

    out.println("</style></head><body>");

    out.println("<div class='container'>");

    // Back link
    out.println("<a href='/' class='back-link'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 19l-7-7 7-7'/></svg>");
    out.println("Back to Dashboard</a>");

    // Header
    out.println("<div class='header'>");
    out.println("<h1>Network Settings</h1>");
    out.println("<p>Configure network connection mode</p>");
    out.println("</div>");

    // Alert
    out.println("<div id='alert' class='alert'></div>");

    // Warning about restart
    out.println("<div id='restartWarning' class='alert warning show'>");
    out.println("Changes will take effect after device restart.</div>");

    // Form
    out.println("<form id='networkForm'>");
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0'/></svg>");
    out.println("Connection Mode</div>");

    // Mode selector
    out.println("<div class='mode-selector'>");

    // Ethernet mode
    out.print("<div id='modeEth' class='mode-btn");
    if (!cfg.useWifi) out.print(" active");
    out.println("' onclick='selectMode(false)'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z'/></svg>");
    out.println("<div class='mode-btn-label'>Ethernet</div>");
    out.println("</div>");

    // WiFi mode
    out.print("<div id='modeWifi' class='mode-btn");
    if (cfg.useWifi) out.print(" active");
    out.println("' onclick='selectMode(true)'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0'/></svg>");
    out.println("<div class='mode-btn-label'>WiFi</div>");
    out.println("</div>");
    out.println("</div>");

    // Hidden mode input
    out.print("<input type='hidden' id='useWifi' name='useWifi' value='");
    out.print(cfg.useWifi ? "true" : "false");
    out.println("'>");

    out.println("</div>");  // card

    // WiFi settings card
    out.print("<div id='wifiSettings' class='card'");
    if (!cfg.useWifi) out.print(" style='display:none'");
    out.println(">");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0'/></svg>");
    out.println("WiFi Configuration</div>");

    // Scan button
    out.println("<div style='margin-bottom:16px'>");
    out.println("<button type='button' class='btn btn-secondary' onclick='scanWifi()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z'/></svg>");
    out.println("Scan Networks</button>");
    out.println("</div>");

    // WiFi list
    out.println("<div id='wifiList' class='wifi-list' style='display:none'>");
    out.println("<div class='wifi-loading'>Scanning...</div>");
    out.println("</div>");

    // SSID input
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Network Name (SSID)</label>");
    out.print("<input type='text' id='ssid' name='ssid' class='form-input' placeholder='Enter or select network' value='");
    out.print(cfg.wifiSsid);
    out.println("'>");
    out.println("</div>");

    // Password input
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Password</label>");
    out.println("<div class='password-wrapper'>");
    out.print("<input type='password' id='password' name='password' class='form-input' placeholder='Enter password' value='");
    out.print(cfg.wifiPassword);
    out.println("'>");
    out.println("<button type='button' class='password-toggle' onclick='togglePassword()'>");
    out.println("<svg id='eyeIcon' fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 12a3 3 0 11-6 0 3 3 0 016 0z'/><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z'/></svg>");
    out.println("</button>");
    out.println("</div>");
    out.println("</div>");

    out.println("</div>");  // WiFi settings card

    // Buttons
    out.println("<div class='btn-group'>");
    out.println("<button type='submit' class='btn btn-primary'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
    out.println("Save & Restart</button>");
    out.println("<button type='button' class='btn btn-secondary' onclick='resetDefaults()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15'/></svg>");
    out.println("Reset</button>");
    out.println("</div>");

    out.println("</form>");

    // Current config info
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z'/></svg>");
    out.println("Current Configuration</div>");
    out.println("<div style='font-size:.75rem;color:#64748b'>");
    out.print("<p>Mode: "); out.print(cfg.useWifi ? "WiFi" : "Ethernet"); out.println("</p>");
    if (cfg.useWifi) {
        out.print("<p>SSID: "); out.print(cfg.wifiSsid); out.println("</p>");
    }
    out.println("</div>");
    out.println("</div>");

    out.println("</div>");  // container

    // JavaScript
    out.println("<script>");

    // Mode selection
    out.println("function selectMode(wifi){");
    out.println("  document.getElementById('useWifi').value=wifi?'true':'false';");
    out.println("  document.getElementById('modeWifi').className='mode-btn'+(wifi?' active':'');");
    out.println("  document.getElementById('modeEth').className='mode-btn'+(wifi?'':' active');");
    out.println("  document.getElementById('wifiSettings').style.display=wifi?'block':'none';");
    out.println("}");

    // Password toggle
    out.println("function togglePassword(){");
    out.println("  var p=document.getElementById('password');");
    out.println("  p.type=p.type==='password'?'text':'password';");
    out.println("}");

    // Show alert
    out.println("function showAlert(msg,type){");
    out.println("  var a=document.getElementById('alert');");
    out.println("  a.textContent=msg;");
    out.println("  a.className='alert '+type+' show';");
    out.println("  setTimeout(function(){a.classList.remove('show');},5000);");
    out.println("}");

    // Scan WiFi networks
    out.println("function scanWifi(){");
    out.println("  var list=document.getElementById('wifiList');");
    out.println("  list.style.display='block';");
    out.println("  list.innerHTML='<div class=\"wifi-loading\">Scanning...</div>';");
    out.println("  fetch('/api/wifi/scan')");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.networks&&d.networks.length>0){");
    out.println("      var html='';");
    out.println("      d.networks.forEach(function(n){");
    out.println("        html+='<div class=\"wifi-item\" onclick=\"selectNetwork(\\''+n.ssid+'\\')\">'+");
    out.println("          '<span class=\"wifi-ssid\">'+n.ssid+'</span>'+");
    out.println("          '<span class=\"wifi-signal\">'+n.rssi+'dBm</span></div>';");
    out.println("      });");
    out.println("      list.innerHTML=html;");
    out.println("    }else{");
    out.println("      list.innerHTML='<div class=\"wifi-empty\">No networks found</div>';");
    out.println("    }");
    out.println("  }).catch(function(e){");
    out.println("    list.innerHTML='<div class=\"wifi-empty\">Scan failed</div>';");
    out.println("  });");
    out.println("}");

    // Select network from list
    out.println("function selectNetwork(ssid){");
    out.println("  document.getElementById('ssid').value=ssid;");
    out.println("  document.querySelectorAll('.wifi-item').forEach(function(el){");
    out.println("    el.classList.remove('selected');");
    out.println("  });");
    out.println("  event.currentTarget.classList.add('selected');");
    out.println("  document.getElementById('password').focus();");
    out.println("}");

    // Form submit
    out.println("document.getElementById('networkForm').addEventListener('submit',function(e){");
    out.println("  e.preventDefault();");
    out.println("  var form=this;");
    out.println("  var useWifi=document.getElementById('useWifi').value==='true';");
    out.println("  if(useWifi&&!document.getElementById('ssid').value){");
    out.println("    showAlert('Please enter WiFi SSID','error');return;");
    out.println("  }");
    out.println("  if(!confirm('Save and restart device?'))return;");
    out.println("  form.classList.add('loading');");
    out.println("  var data={");
    out.println("    useWifi:useWifi,");
    out.println("    ssid:document.getElementById('ssid').value,");
    out.println("    password:document.getElementById('password').value");
    out.println("  };");
    out.println("  fetch('/api/network',{");
    out.println("    method:'POST',");
    out.println("    headers:{'Content-Type':'application/json'},");
    out.println("    body:JSON.stringify(data)");
    out.println("  }).then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    form.classList.remove('loading');");
    out.println("    if(d.success){");
    out.println("      showAlert('Configuration saved! Restarting...','success');");
    out.println("      setTimeout(function(){location.href='/';},3000);");
    out.println("    }else{showAlert('Error: '+d.error,'error');}");
    out.println("  }).catch(function(e){");
    out.println("    form.classList.remove('loading');");
    out.println("    showAlert('Network error','error');");
    out.println("  });");
    out.println("});");

    // Reset defaults
    out.println("function resetDefaults(){");
    out.println("  if(!confirm('Reset network settings to defaults?'))return;");
    out.println("  fetch('/api/network/reset',{method:'POST'})");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.success){location.reload();}");
    out.println("    else{showAlert('Error: '+d.error,'error');}");
    out.println("  }).catch(function(e){showAlert('Network error','error');});");
    out.println("}");

    out.println("</script>");
    out.println("</body></html>");
}

} // namespace WebPageNetworkSettings

#endif // WEBPAGE_NETWORK_SETTINGS_H
