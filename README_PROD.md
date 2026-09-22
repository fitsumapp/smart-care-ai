# MedPulse Smart-Care AI — Deployment Guide

ይህ መመሪያ ሲስተሙን በ Raspberry Pi ላይ እንዴት እንደሚጭኑ ደረጃ በደረጃ ያብራራል።

## 1. ፋይሎቹን ማስተላለፍ (Transfer)
ዊንዶውስ ላይ ተርሚናል በመክፈት ፋይሎቹን ወደ ፒአይ ይላኩ፡
```bash
scp -r c:\ai_nurse_system pi@<RASPBERRY_IP>:/home/pi/
```

## 2. Environment Setup
ፒአይ ላይ ገብተው `.env` ፋይሉን ያስተካክሉ፡
```bash
cd /home/pi/ai_nurse_system
nano .env
```
እዚህ ውስጥ፡
- `DEBUG=False` ማድረጉን እርግጠኛ ይሁኑ።
- `ALLOWED_HOSTS` ላይ የፒአይ አይፒ አድራሻን ይጨምሩ።
- PostgreSQL የሚጠቀሙ ከሆነ የዳታቤዝ ዝርዝሮችን ያስገቡ።

## 3. Deployment Script
ሲስተሙን በራስ-ሰር ለመጫን ይህንን ስክሪፕት ያሂዱ፡
```bash
bash deploy_to_pi.sh
```
ይህ ስክሪፕት፡
- አስፈላጊ ሶፍትዌሮችን ይጭናል።
- ዳታቤዙን ያዘጋጃል።
- ሲስተሙን እንደ Service ይመዘግባል።

## 4. ሲስተሙን መቆጣጠር
ሲስተሙ በትክክል መስራቱን ለማየት፡
```bash
sudo systemctl status medpulse      # ለዌብሳይቱ
sudo systemctl status serial_bridge  # ለሃርድዌሩ
```

ማንኛውም ስህተት ካለ በ `logs/` ፎልደር ውስጥ ማየት ይችላሉ።
