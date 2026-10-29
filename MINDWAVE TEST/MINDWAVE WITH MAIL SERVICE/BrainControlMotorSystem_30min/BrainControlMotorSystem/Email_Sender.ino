/************************************************************
   EMAIL_SENDER.INO
   SMTP sending via ESP-Mail-Client. The #include, SMTP_HOST/
   PORT/USER/PASS #defines, and the shared `smtp` session
   object all live in BrainControlMotorSystem.ino (the main
   tab) since that's what compiles first — see that file's
   EMAIL (SMTP) section to fill in your credentials.
************************************************************/

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
  if (status.success()) {
    emailLastResult = true;
  }
  emailInProgress = false;
}

/************************************************************
   📧 POST /send-email  — sends the result summary via SMTP
   using ESP-Mail-Client. Chosen over a webhook because it
   needs no external server to maintain: everything runs on
   the ESP32 with a Gmail (or any SMTP relay) App Password.
************************************************************/
bool sendResultEmailNow() {
  if (WiFi.status() != WL_CONNECTED) return false;

  smtp.debug(1);
  smtp.callback(smtpCallback);

  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = SMTP_USER;
  config.login.password = SMTP_PASS;
  config.login.user_domain = "";

  // Required for TLS: without correct system time, the certificate
  // date-validity check fails and the handshake is silently rejected.
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 0;
  config.time.day_light_offset = 0;

  SMTP_Message message;
  message.sender.name = SMTP_SENDER_NAME;
  message.sender.email = SMTP_USER;
  message.subject = "Your Focus Session Result";
  message.addRecipient(userName.c_str(), userEmail.c_str());

  String body = "Hi " + userName + ",<br><br>";
  body += "Here is your focus session summary:<br><br>";
  body += "Session length: " + String(sessionMinutes) + " min<br>";
  body += "Motor-on time: " + String(motorOnSeconds) + " sec<br>";
  body += "Average attention: " + String(avgAttentionResult) + "%<br>";
  body += "Peak attention: " + String(peakAttentionResult) + "%<br><br>";
  body += "— Brain Control Motor System";

  message.html.content = body.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  if (!smtp.connect(&config)) {
    Serial.println("SMTP connect() failed:");
    Serial.println(smtp.errorReason());
    return false;
  }
  bool ok = MailClient.sendMail(&smtp, &message);
  if (!ok) {
    Serial.println("sendMail() failed:");
    Serial.println(smtp.errorReason());
  }
  smtp.closeSession();
  return ok;
}

void handleSendEmail() {
  if (appState != STATE_RESULT) {
    server.send(409, "text/plain", "No completed session to email");
    return;
  }

  emailInProgress = true;
  emailAttempted = true;
  emailLastResult = false;

  bool ok = sendResultEmailNow();
  emailLastResult = ok;
  emailInProgress = false;

  if (ok) {
    showText("Email Sent", userEmail);
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    showText("Email Failed", "Check SMTP config");
    server.send(502, "application/json", "{\"ok\":false}");
  }
}
