# 🌐 MedPulse Smart-Care AI — የ cPanel Shared Hosting የማሰማሪያ መመሪያ (Deployment Guide)

ይህ መመሪያ የ MedPulse Smart-Care AI ሲስተምን በማንኛውም **cPanel Shared Hosting (Python App ድጋፍ ባለው)** ላይ በቀላሉ እና ደረጃ በደረጃ እንዴት መጫን እንደሚችሉ ያብራራል።

---

## 📋 ቅድመ-ዝግጅት (Requirements)
1. **cPanel Hosting Account** (ከ **"Setup Python App"** ወይም **"Passenger"** ድጋፍ ጋር)።
2. የዶሜይን ስም (ለምሳሌ፦ `hospital-nurse.com` ወይም `subdomain.yourdomain.com`) ከነፃ **SSL (HTTPS)** ጋር።

---

## 🚀 ደረጃ 1፦ ፋይሎችን ወደ cPanel ማስተላለፍ (Upload Files)

1. ኮምፒውተርዎ ላይ ያለውን የ `ai_nurse_system` ፎልደር (ከ `venv` እና `.git` ውጪ) በ **ZIP** ይቋጥሩት።
2. ወደ **cPanel ➔ File Manager** ይግቡ።
3. ፋይሉን በ `public_html` ወይም ለፕሮጀክቱ በተዘጋጀ ፎልደር ውስጥ (ለምሳሌ `/home/username/ai_nurse_system`) **Upload** ያድርጉ እና **Extract** ያድርጉት።

---

## 🗄️ ደረጃ 2፦ MySQL ዳታቤዝ ማዘጋጀት (Create MySQL Database)

1. በ cPanel ውስጥ **"MySQL Database Wizard"** ይክፈቱ።
2. አዲስ ዳታቤዝ ይፍጠሩ (ለምሳሌ፦ `cpaneluser_medpulse_db`)።
3. አዲስ ተጠቃሚ (User) እና ጠንካራ የይለፍ ቃል (Password) ይፍጠሩ (ለምሳሌ፦ `cpaneluser_nurseadmin`)።
4. ተጠቃሚውን ከዳታቤዙ ጋር በማገናኘት **"ALL PRIVILEGES"** የሚለውን ምልክት ያድርጉ እና **Make Changes** ይበሉ።

---

## 🐍 ደረጃ 3፦ Python App በ cPanel መፍጠር (Setup Python App)

1. በ cPanel ውስጥ **"Setup Python App"** የሚለውን ይክፈቱ።
2. **"Create Application"** የሚለውን ይጫኑ፦
   - **Python version:** `3.10`, `3.11` ወይም `3.12` ይምረጡ።
   - **Application root:** የፕሮጀክቱ ፎልደር መንገድ (ለምሳሌ፦ `ai_nurse_system`)።
   - **Application URL:** ዶሜይንዎን ይምረጡ (ለምሳሌ፦ `yourdomain.com`)።
   - **Application startup file:** `passenger_wsgi.py`
   - **Application Entry point:** `application`
3. **"CREATE"** የሚለውን ቁልፍ ይጫኑ።

---

## ⚙️ ደረጃ 4፦ የ `.env` ፋይል ማስተካከል (Environment Configuration)

በ cPanel File Manager ውስጥ የ `.env.example` ፋይሉን ስም ወደ `.env` ይቀይሩት (Rename)፤ ከዚያም ይክፈቱትና የሚከተሉትን ያስተካክሉ፦

```env
# Security
SECRET_KEY=የፈለጉትን_ረጅም_ምስጢራዊ_ቁልፍ_ያስገቡ_12345!@#$%
DEBUG=False
ALLOWED_HOSTS=yourdomain.com,www.yourdomain.com
CSRF_TRUSTED_ORIGINS=https://yourdomain.com,https://www.yourdomain.com

# Database (ደረጃ 2 ላይ የፈጠሩትን የ MySQL መረጃ ያስገቡ)
DB_ENGINE=django.db.backends.mysql
DB_NAME=cpaneluser_medpulse_db
DB_USER=cpaneluser_nurseadmin
DB_PASSWORD=የዳታቤዙ_የይለፍ_ቃል
DB_HOST=localhost
DB_PORT=3306

# Hardware Security
STATION_API_KEY=medpulse-station-secret-key-2026
```

---

## 📦 ደረጃ 5፦ ፓኬጆችን መጫን (Install Dependencies)

1. በ cPanel "Setup Python App" ገጽ ላይ ከላይ የሚገኘውን **"Command for entering to the virtual environment"** ኮፒ ያድርጉ።
2. በ cPanel ውስጥ **"Terminal"** ይክፈቱ እና ኮፒ ያደረጉትን ትዕዛዝ Paste አድርገው አስጀምሩት (ወደ Virtualenv ይገባል)።
3. የሚከተሉትን ትዕዛዞች በቅደም ተከተል ያሂዱ፦
```bash
cd ~/ai_nurse_system
pip install --upgrade pip
pip install -r requirements.txt
```

---

## 🗃️ ደረጃ 6፦ ዳታቤዙን መገንባት እና አስተዳዳሪ መፍጠር (Migrate & Superuser)

ተርሚናሉ ላይ ሆነው የሚከተሉትን ያሂዱ፦

```bash
# 1. ዳታቤዝ ቴብሎችን መፍጠር
python manage.py migrate

# 2. የዋና አስተዳዳሪ (Supervisor) አካውንት መፍጠር
python manage.py createsuperuser

# 3. የስታቲክ ፋይሎችን መሰብሰብ
python manage.py collectstatic --noinput
```

---

## 🔄 ደረጃ 7፦ አፕሊኬሽኑን ማስነሳት (Restart Python App)

1. ወደ **"Setup Python App"** ገጽ ይመለሱ።
2. የፈጠሩትን አፕሊኬሽን አግኝተው **"RESTART"** የሚለውን ቁልፍ ይጫኑ።

---

## ✅ ደረጃ 8፦ ማረጋገጥ እና መሞከር (Verification)

1. በ Browserዎ `https://yourdomain.com` ይክፈቱ ➔ የ MedPulse መግቢያ (Login) ገጽ መምጣቱን ያረጋግጡ።
2. በፈጠሩት የሱፐርቫይዘር አካውንት ገብተው የነርስ ጣቢያዎችን (Nurse Stations) እና ክፍሎችን ያዋቅሩ።
3. የኮምፒውተር ደንበኛ ስክሪፕቶችን (`client_bridges/master_bridge.py`) `.env` ላይ ዶሜይንዎን በማስገባት ያስጀምሩ።

---

🎉 **እንኳን ደስ አለዎት! የ MedPulse Smart-Care AI ሲስተምዎ በ cPanel Shared Hosting ላይ በተሟላ መልኩ ተጭኗል!**
