#ifndef WEBPAGE_SETTINGS_H
#define WEBPAGE_SETTINGS_H

/**
 * @file webpage_settings.h
 * @brief Settings page renderer for server configuration
 */

#include <Arduino.h>
#include "../config.h"

// Default aman — lihat catatan di webserver_module.h. Diulang di sini supaya
// header ini benar juga saat di-include lebih dulu dari TU lain.
#ifndef OTA_REQUIRE_TOKEN
#define OTA_REQUIRE_TOKEN 1
#endif

#include "config_manager.h"

namespace WebPageSettings {

inline void render(Print& out, const ConfigManager& configMgr) {
    const WebhookConfig& cfg = configMgr.getConfig();
    const TimingConfig& tcfg = configMgr.getTimingConfig();

    // HTTP Response
    out.println("HTTP/1.1 200 OK");
    out.println("Content-Type: text/html");
    out.println("Connection: close");
    out.println();

    // HTML
    out.println("<!DOCTYPE html><html lang='en'><head>");
    out.println("<meta charset='UTF-8'>");
    out.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    out.println("<title>Server Settings - PELNI GPS Tracker</title>");
    out.println("<style>");

    // CSS
    out.println("*{margin:0;padding:0;box-sizing:border-box}");
    out.println("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;padding:20px}");
    out.println(".container{max-width:960px;margin:0 auto}");
    out.println(".cards{display:grid;grid-template-columns:repeat(2,1fr);gap:16px;align-items:stretch}");
    out.println(".cards .card{margin-bottom:0}");
    out.println("@media(max-width:720px){.cards{grid-template-columns:1fr}.container{max-width:520px}}");
    out.println("@media(max-width:480px){.form-row{grid-template-columns:1fr}}");
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
    out.println(".form-row{display:grid;grid-template-columns:1fr 1fr;gap:12px}");
    out.println(".btn{padding:12px 24px;border:none;border-radius:8px;font-size:.875rem;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;justify-content:center;gap:8px}");
    out.println(".btn-primary{background:#38bdf8;color:#0f172a}.btn-primary:hover{background:#0ea5e9}");
    out.println(".btn-secondary{background:#334155;color:#e2e8f0}.btn-secondary:hover{background:#475569}");
    out.println(".btn-danger{background:#ef4444;color:#fff}.btn-danger:hover{background:#dc2626}");
    out.println(".btn-group{display:flex;gap:12px;margin-top:20px}");
    out.println(".btn svg{width:16px;height:16px}");
    out.println(".alert{padding:12px 16px;border-radius:8px;margin-bottom:16px;font-size:.875rem;display:none}");
    out.println(".alert.success{background:rgba(34,197,94,.2);border:1px solid rgba(34,197,94,.3);color:#4ade80}");
    out.println(".alert.error{background:rgba(239,68,68,.2);border:1px solid rgba(239,68,68,.3);color:#f87171}");
    out.println(".alert.show{display:block}");
    out.println(".back-link{display:inline-flex;align-items:center;gap:6px;color:#64748b;text-decoration:none;font-size:.875rem;margin-bottom:20px}");
    out.println(".back-link:hover{color:#94a3b8}");
    out.println(".back-link svg{width:16px;height:16px}");
    out.println(".toggle{display:flex;align-items:center;gap:12px}");
    out.println(".toggle-switch{position:relative;width:48px;height:26px;background:#334155;border-radius:13px;cursor:pointer;transition:background .2s}");
    out.println(".toggle-switch.active{background:#22c55e}");
    out.println(".toggle-switch::after{content:'';position:absolute;top:3px;left:3px;width:20px;height:20px;background:#fff;border-radius:50%;transition:transform .2s}");
    out.println(".toggle-switch.active::after{transform:translateX(22px)}");
    out.println(".toggle-label{font-size:.875rem;color:#e2e8f0}");
    out.println(".loading{opacity:.6;pointer-events:none}");
    out.println(".form-hint{font-size:.7rem;color:#64748b;margin-top:4px}");

    out.println("</style></head><body>");

    out.println("<div class='container'>");

    // Back link
    out.println("<a href='/' class='back-link'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 19l-7-7 7-7'/></svg>");
    out.println("Back to Dashboard</a>");

    // Header
    out.println("<div class='header'>");
    out.println("<h1>Server Settings</h1>");
    out.println("<p>Configure server endpoint for GPS data</p>");
    out.println("</div>");

    // Alert
    out.println("<div id='alert' class='alert'></div>");

    // Cards grid (2 kolom di desktop, 1 kolom di layar kecil)
    out.println("<div class='cards'>");

    // Form (display:contents -> kartu di dalamnya ikut jadi grid item .cards)
    out.println("<form id='configForm' style='display:contents'>");
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z'/><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 12a3 3 0 11-6 0 3 3 0 016 0z'/></svg>");
    out.println("Server Configuration</div>");

    // Enable toggle
    out.println("<div class='form-group'>");
    out.println("<div class='toggle'>");
    out.print("<div id='enableToggle' class='toggle-switch");
    if (cfg.enabled) out.print(" active");
    out.println("' onclick='toggleEnabled()'></div>");
    out.println("<span class='toggle-label'>Enable Server Sync</span>");
    out.println("</div>");
    out.print("<input type='hidden' id='enabled' name='enabled' value='");
    out.print(cfg.enabled ? "true" : "false");
    out.println("'>");
    out.println("</div>");

    // Host
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Host / IP Address</label>");
    out.print("<input type='text' id='host' name='host' class='form-input' placeholder='example.com or 192.168.1.100' value='");
    out.print(cfg.host);
    out.println("' required>");
    out.println("</div>");

    // Port & Path
    out.println("<div class='form-row'>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Port</label>");
    out.print("<input type='number' id='port' name='port' class='form-input' placeholder='80' min='1' max='65535' value='");
    out.print(cfg.port);
    out.println("' required>");
    out.println("</div>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Path</label>");
    out.print("<input type='text' id='path' name='path' class='form-input' placeholder='/api/webhook' value='");
    out.print(cfg.path);
    out.println("' required>");
    out.println("</div>");
    out.println("</div>");

    // Ping Path & Sync Path
    out.println("<div class='form-row'>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Ping Path</label>");
    out.print("<input type='text' id='pingPath' name='pingPath' class='form-input' placeholder='/health' value='");
    out.print(cfg.pingPath);
    out.println("' required>");
    out.println("</div>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Sync Path</label>");
    out.print("<input type='text' id='syncPath' name='syncPath' class='form-input' placeholder='/api/device/sync' value='");
    out.print(cfg.syncPath);
    out.println("' required>");
    out.println("</div>");
    out.println("</div>");

    // Buttons
    out.println("<div class='btn-group'>");
    out.println("<button type='submit' class='btn btn-primary'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
    out.println("Save</button>");
    out.println("<button type='button' class='btn btn-secondary' onclick='resetDefaults()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15'/></svg>");
    out.println("Reset</button>");
    out.println("</div>");

    out.println("</div>");  // card (server config)

    // Timing configuration card (inside same form -> single Save submits both)
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z'/></svg>");
    out.println("Timing Configuration</div>");

    out.println("<div class='form-row'>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Send Interval (GPS fix)</label>");
    out.print("<input type='number' id='sendIntervalNormal' name='sendIntervalNormal' class='form-input' min='1000' step='1000' value='");
    out.print(tcfg.sendIntervalNormal);
    out.println("' required>");
    out.println("<div class='form-hint'>ms between sends when GPS has a fix (min 1000)</div>");
    out.println("</div>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Send Interval (no fix)</label>");
    out.print("<input type='number' id='sendIntervalNoFix' name='sendIntervalNoFix' class='form-input' min='1000' step='1000' value='");
    out.print(tcfg.sendIntervalNoFix);
    out.println("' required>");
    out.println("<div class='form-hint'>ms between sends when GPS has no fix (min 1000)</div>");
    out.println("</div>");
    out.println("</div>");

    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>HTTP Timeout</label>");
    out.print("<input type='number' id='httpTimeout' name='httpTimeout' class='form-input' min='500' step='100' value='");
    out.print(tcfg.httpTimeout);
    out.println("' required>");
    out.println("<div class='form-hint'>ms to wait for HTTP response before giving up (min 500)</div>");
    out.println("</div>");

    out.println("</div>");  // card (timing)

    out.println("</form>");

    // Connection & Sync card (combined)
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z'/></svg>");
    out.println("Connection &amp; Sync</div>");
    out.println("<div style='font-size:.75rem;color:#64748b;margin-bottom:16px'>Check = HTTP GET to Ping Path (no save needed). Sync = POST device identity (IP, MAC, Device ID, Firmware) to saved Sync Path (save first).</div>");
    out.println("<div id='pingStatus' style='font-size:.875rem;color:#94a3b8;margin-bottom:8px'>Connection: not checked</div>");
    out.println("<div id='syncStatus' style='font-size:.875rem;color:#94a3b8;margin-bottom:16px'>Sync: idle</div>");
    out.println("<div class='btn-group'>");
    out.println("<button type='button' class='btn btn-secondary' onclick='checkConnection()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M13 10V3L4 14h7v7l9-11h-7z'/></svg>");
    out.println("Check Connection</button>");
    out.println("<button type='button' class='btn btn-primary' onclick='syncToServer()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
    out.println("Sync Now</button>");
    out.println("</div>");
    out.println("</div>");

    // Server Trust (TOFU) card
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z'/></svg>");
    out.println("Server Trust (TOFU)</div>");
    out.println("<div style='font-size:.75rem;color:#64748b;margin-bottom:16px'>Server pertama yang terverifikasi lewat discovery di-&quot;pin&quot;. Kalau server sah pindah IP/port dan device menolak reply baru (fingerprint mismatch), Reset Trust lalu jalankan Re-discover.</div>");
    out.println("<div id='trustStatus' style='font-size:.875rem;color:#94a3b8;margin-bottom:16px'>Trust: loading...</div>");
    out.println("<div class='btn-group'>");
    out.println("<button type='button' class='btn btn-danger' onclick='resetTrust()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M13.875 18.825A10.05 10.05 0 0112 19c-4.478 0-8.268-2.943-9.543-7a9.97 9.97 0 011.563-3.029m5.858.908a3 3 0 114.243 4.243M9.878 9.878l4.242 4.242M9.88 9.88l-3.29-3.29m7.532 7.532l3.29 3.29M3 3l3.59 3.59m0 0A9.953 9.953 0 0112 5c4.478 0 8.268 2.943 9.543 7a10.025 10.025 0 01-4.132 5.411m0 0L21 21'/></svg>");
    out.println("Reset Trust</button>");
    out.println("<button type='button' class='btn btn-secondary' onclick='rediscover()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15'/></svg>");
    out.println("Re-discover</button>");
    out.println("</div>");
    out.println("</div>");  // card (server trust)

    // Manual Firmware Update card
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12'/></svg>");
    out.println("Manual Firmware Update</div>");
    out.println("<div style='font-size:.75rem;color:#64748b;margin-bottom:16px'>Upload firmware .bin langsung ke device (OTA). Device reboot otomatis setelah flashing; kalau gagal online, rollback ke firmware lama. Jangan matikan daya selama proses.</div>");
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>Firmware (.bin)</label>");
    out.println("<input type='file' id='fwFile' accept='.bin' class='form-input'>");
#if !OTA_REQUIRE_TOKEN
    out.println("<div class='form-hint'>OTA token sedang dinonaktifkan &mdash; upload dari dashboard ini tidak perlu token.</div>");
#endif
    out.println("</div>");
#if OTA_REQUIRE_TOKEN
    out.println("<div class='form-group'>");
    out.println("<label class='form-label'>OTA Token</label>");
    out.println("<input type='text' id='fwToken' class='form-input' placeholder='X-Update-Token' autocomplete='off'>");
    out.println("<div class='form-hint'>Token unik device &mdash; dari serial log saat boot, atau dari node_local.</div>");
    out.println("</div>");
#endif
    out.println("<div id='fwBarWrap' style='display:none;height:8px;background:#0f172a;border-radius:4px;overflow:hidden;margin:4px 0 12px'>");
    out.println("<div id='fwBar' style='height:100%;width:0%;background:#38bdf8;transition:width .2s'></div>");
    out.println("</div>");
    out.println("<div id='fwStatus' style='font-size:.875rem;color:#94a3b8;margin-bottom:16px'>Firmware: idle</div>");
    out.println("<div class='btn-group'>");
    out.println("<button type='button' class='btn btn-primary' onclick='uploadFirmware()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-8l-4-4m0 0L8 8m4-4v12'/></svg>");
    out.println("Upload &amp; Flash</button>");
    out.println("</div>");
    out.println("</div>");  // card (manual firmware)

    out.println("</div>");  // cards grid

    out.println("</div>");  // container

    // JavaScript
    out.println("<script>");

    out.println("function toggleEnabled(){");
    out.println("  var t=document.getElementById('enableToggle');");
    out.println("  var i=document.getElementById('enabled');");
    out.println("  t.classList.toggle('active');");
    out.println("  i.value=t.classList.contains('active')?'true':'false';");
    out.println("}");

    out.println("function showAlert(msg,type){");
    out.println("  var a=document.getElementById('alert');");
    out.println("  a.textContent=msg;");
    out.println("  a.className='alert '+type+' show';");
    out.println("  setTimeout(function(){a.classList.remove('show');},3000);");
    out.println("}");

    out.println("document.getElementById('configForm').addEventListener('submit',function(e){");
    out.println("  e.preventDefault();");
    out.println("  var form=this;");
    out.println("  form.classList.add('loading');");
    out.println("  var data={");
    out.println("    host:document.getElementById('host').value,");
    out.println("    port:parseInt(document.getElementById('port').value),");
    out.println("    path:document.getElementById('path').value,");
    out.println("    pingPath:document.getElementById('pingPath').value,");
    out.println("    syncPath:document.getElementById('syncPath').value,");
    out.println("    enabled:document.getElementById('enabled').value==='true',");
    out.println("    sendIntervalNormal:parseInt(document.getElementById('sendIntervalNormal').value,10),");
    out.println("    sendIntervalNoFix:parseInt(document.getElementById('sendIntervalNoFix').value,10),");
    out.println("    httpTimeout:parseInt(document.getElementById('httpTimeout').value,10)");
    out.println("  };");
    out.println("  fetch('/api/config',{");
    out.println("    method:'POST',");
    out.println("    headers:{'Content-Type':'application/json'},");
    out.println("    body:JSON.stringify(data)");
    out.println("  }).then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    form.classList.remove('loading');");
    out.println("    if(d.success){showAlert('Configuration saved!','success');}");
    out.println("    else{showAlert('Error: '+d.error,'error');}");
    out.println("  }).catch(function(e){");
    out.println("    form.classList.remove('loading');");
    out.println("    showAlert('Network error','error');");
    out.println("  });");
    out.println("});");

    out.println("function resetDefaults(){");
    out.println("  if(!confirm('Reset to default values?'))return;");
    out.println("  fetch('/api/config/reset',{method:'POST'})");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.success){location.reload();}");
    out.println("    else{showAlert('Error: '+d.error,'error');}");
    out.println("  }).catch(function(e){showAlert('Network error','error');});");
    out.println("}");

    out.println("function checkConnection(){");
    out.println("  var s=document.getElementById('pingStatus');");
    out.println("  s.textContent='Connection: checking...';s.style.color='#94a3b8';");
    out.println("  var data={host:document.getElementById('host').value,port:parseInt(document.getElementById('port').value),path:document.getElementById('pingPath').value};");
    out.println("  fetch('/api/server/ping',{");
    out.println("    method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)");
    out.println("  }).then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.reachable){s.textContent='Connection: reachable';s.style.color='#4ade80';}");
    out.println("    else{s.textContent='Connection: unreachable';s.style.color='#f87171';}");
    out.println("  }).catch(function(e){s.textContent='Connection: error';s.style.color='#f87171';});");
    out.println("}");

    out.println("function syncToServer(){");
    out.println("  var s=document.getElementById('syncStatus');");
    out.println("  s.textContent='Sync: syncing...';s.style.color='#94a3b8';");
    out.println("  fetch('/api/server/sync',{method:'POST'})");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.success){s.textContent='Sync: synced (HTTP '+d.statusCode+')';s.style.color='#4ade80';}");
    out.println("    else{s.textContent='Sync: failed (HTTP '+d.statusCode+')';s.style.color='#f87171';}");
    out.println("  }).catch(function(e){s.textContent='Sync: error';s.style.color='#f87171';});");
    out.println("}");

    out.println("function loadTrustStatus(){");
    out.println("  var s=document.getElementById('trustStatus');");
    out.println("  fetch('/api/server/status').then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.trustLocked){s.innerHTML='Trust: <span style=\"color:#fbbf24;font-weight:600\">LOCKED</span> \\u2192 '+d.host+':'+d.port+' <span style=\"color:#64748b\">(fp '+d.trustFpShort+'\\u2026)</span>';}");
    out.println("    else{s.innerHTML='Trust: <span style=\"color:#4ade80;font-weight:600\">unlocked</span> <span style=\"color:#64748b\">(belum ada server yang di-pin)</span>';}");
    out.println("  }).catch(function(e){s.textContent='Trust: status unavailable';});");
    out.println("}");

    out.println("function resetTrust(){");
    out.println("  if(!confirm('Reset TOFU trust? Device akan menerima server discovery baru pada attempt berikutnya.'))return;");
    out.println("  fetch('/api/server/reset-trust',{method:'POST'})");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){");
    out.println("    if(d.success){showAlert('Trust di-reset. Jalankan Re-discover untuk pin server baru.','success');setTimeout(loadTrustStatus,500);}");
    out.println("    else{showAlert('Error: '+(d.error||'reset gagal'),'error');}");
    out.println("  }).catch(function(e){showAlert('Network error','error');});");
    out.println("}");

    out.println("function rediscover(){");
    out.println("  showAlert('Discovery di-queue... status akan diperbarui beberapa detik lagi.','success');");
    out.println("  fetch('/api/server/discover',{method:'POST'})");
    out.println("  .then(function(r){return r.json();})");
    out.println("  .then(function(d){setTimeout(loadTrustStatus,3500);})");
    out.println("  .catch(function(e){showAlert('Network error','error');});");
    out.println("}");

    out.println("var fwBusy=false;");
    out.println("function uploadFirmware(){");
    out.println("  if(fwBusy)return;");
    out.println("  var f=document.getElementById('fwFile').files[0];");
#if OTA_REQUIRE_TOKEN
    out.println("  var token=document.getElementById('fwToken').value.trim();");
#endif
    out.println("  var s=document.getElementById('fwStatus');");
    out.println("  var bar=document.getElementById('fwBar');");
    out.println("  var wrap=document.getElementById('fwBarWrap');");
    out.println("  if(!f){showAlert('Pilih file .bin dulu','error');return;}");
    out.println("  if(f.name.toLowerCase().slice(-4)!=='.bin'){showAlert('File harus .bin','error');return;}");
#if OTA_REQUIRE_TOKEN
    out.println("  if(!token){showAlert('Masukkan OTA token','error');return;}");
#endif
    out.println("  if(!confirm('Upload '+f.name+' ('+Math.round(f.size/1024)+' KB)? Device reboot setelah flashing.'))return;");
    out.println("  fwBusy=true;wrap.style.display='block';bar.style.width='0%';");
    out.println("  s.textContent='Uploading... 0%';s.style.color='#94a3b8';");
    out.println("  var sent=false;");
    out.println("  var xhr=new XMLHttpRequest();");
    out.println("  xhr.open('POST','/api/firmware/update',true);");
#if OTA_REQUIRE_TOKEN
    out.println("  xhr.setRequestHeader('X-Update-Token',token);");
#endif
    out.println("  xhr.setRequestHeader('Content-Type','application/octet-stream');");
    out.println("  xhr.upload.onprogress=function(e){");
    out.println("    if(!e.lengthComputable)return;");
    out.println("    var pct=Math.round(e.loaded/e.total*100);");
    out.println("    bar.style.width=pct+'%';");
    out.println("    if(pct>=100){sent=true;s.textContent='Flashing di device... jangan matikan daya.';}");
    out.println("    else{s.textContent='Uploading... '+pct+'%';}");
    out.println("  };");
    out.println("  xhr.onload=function(){");
    out.println("    if(xhr.status===200){fwSuccess(s);}");
    out.println("    else{fwBusy=false;s.textContent='Gagal (HTTP '+xhr.status+'): '+xhr.responseText;s.style.color='#f87171';}");
    out.println("  };");
    // Koneksi putus SETELAH byte terkirim penuh = device sudah reboot untuk flashing → perlakukan sukses.
    out.println("  xhr.onerror=function(){");
    out.println("    if(sent){fwSuccess(s);}");
    out.println("    else{fwBusy=false;s.textContent='Upload error (koneksi terputus sebelum selesai)';s.style.color='#f87171';}");
    out.println("  };");
    out.println("  xhr.send(f);");
    out.println("}");

    // Poll /api/device/status until the rebooted device answers, then show the new version.
    out.println("function fwSuccess(s){");
    out.println("  s.textContent='Flashing OK. Device reboot, memverifikasi versi...';s.style.color='#4ade80';");
    out.println("  var tries=0;");
    out.println("  var iv=setInterval(function(){");
    out.println("    tries++;");
    out.println("    fetch('/api/device/status').then(function(r){return r.json();})");
    out.println("    .then(function(d){clearInterval(iv);fwBusy=false;s.innerHTML='Selesai \\u2014 firmware <b>'+d.version+'</b>, mengalihkan ke dashboard...';setTimeout(function(){location.href='/';},1500);})");
    out.println("    .catch(function(e){if(tries>=15){clearInterval(iv);fwBusy=false;s.textContent='Device reboot \\u2014 belum merespons, silakan refresh halaman.';}});");
    out.println("  },2000);");
    out.println("}");

    out.println("loadTrustStatus();");

    out.println("</script>");
    out.println("</body></html>");
}

} // namespace WebPageSettings

#endif // WEBPAGE_SETTINGS_H
