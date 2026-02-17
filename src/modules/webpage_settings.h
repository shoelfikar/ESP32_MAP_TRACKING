#ifndef WEBPAGE_SETTINGS_H
#define WEBPAGE_SETTINGS_H

/**
 * @file webpage_settings.h
 * @brief Settings page renderer for webhook configuration
 */

#include <Arduino.h>
#include "../config.h"
#include "config_manager.h"

namespace WebPageSettings {

inline void render(Print& out, const ConfigManager& configMgr) {
    const WebhookConfig& cfg = configMgr.getConfig();

    // HTTP Response
    out.println("HTTP/1.1 200 OK");
    out.println("Content-Type: text/html");
    out.println("Connection: close");
    out.println();

    // HTML
    out.println("<!DOCTYPE html><html lang='en'><head>");
    out.println("<meta charset='UTF-8'>");
    out.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    out.println("<title>Settings - PELNI GPS Tracker</title>");
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

    out.println("</style></head><body>");

    out.println("<div class='container'>");

    // Back link
    out.println("<a href='/' class='back-link'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 19l-7-7 7-7'/></svg>");
    out.println("Back to Dashboard</a>");

    // Header
    out.println("<div class='header'>");
    out.println("<h1>Webhook Settings</h1>");
    out.println("<p>Configure webhook endpoint for GPS data</p>");
    out.println("</div>");

    // Alert
    out.println("<div id='alert' class='alert'></div>");

    // Form
    out.println("<form id='configForm'>");
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z'/><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 12a3 3 0 11-6 0 3 3 0 016 0z'/></svg>");
    out.println("Webhook Configuration</div>");

    // Enable toggle
    out.println("<div class='form-group'>");
    out.println("<div class='toggle'>");
    out.print("<div id='enableToggle' class='toggle-switch");
    if (cfg.enabled) out.print(" active");
    out.println("' onclick='toggleEnabled()'></div>");
    out.println("<span class='toggle-label'>Enable Webhook</span>");
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

    // Buttons
    out.println("<div class='btn-group'>");
    out.println("<button type='submit' class='btn btn-primary'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
    out.println("Save</button>");
    out.println("<button type='button' class='btn btn-secondary' onclick='resetDefaults()'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15'/></svg>");
    out.println("Reset</button>");
    out.println("</div>");

    out.println("</div>");  // card
    out.println("</form>");

    // Default values info
    out.println("<div class='card'>");
    out.println("<div class='card-title'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z'/></svg>");
    out.println("Default Values</div>");
    out.println("<div style='font-size:.75rem;color:#64748b'>");
    out.print("<p>Host: "); out.print(SERVER_HOST); out.println("</p>");
    out.print("<p>Port: "); out.print(SERVER_PORT); out.println("</p>");
    out.print("<p>Path: "); out.print(SERVER_PATH); out.println("</p>");
    out.println("</div>");
    out.println("</div>");

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
    out.println("    enabled:document.getElementById('enabled').value==='true'");
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

    out.println("</script>");
    out.println("</body></html>");
}

} // namespace WebPageSettings

#endif // WEBPAGE_SETTINGS_H
