# ChatRoom – Socket Programming Assignment
## Data Transmission with Error Detection (Client1 → Server → Client2)

Bu proje, veri haberleşmesinde kullanılan **hata tespit yöntemlerini** pratik olarak göstermek için geliştirilmiş 3 bileşenli bir socket uygulamasıdır:

- **Client 1 (Data Sender):** Kullanıcıdan metin alır, seçilen yönteme göre kontrol bilgisini üretir ve paketi sunucuya yollar.
- **Server (Intermediate Node + Data Corruptor):** Paketi alır, **DATA** kısmına hata enjekte eder (paket formatını bozmadan) ve Client 2’ye iletir.
- **Client 2 (Receiver + Error Checker):** Paketi alır, yönteme göre kontrol bilgisini yeniden hesaplar ve gelen kontrol bilgisiyle karşılaştırarak sonucu yazdırır.

> Not: Projede tek bir `ChatClient` uygulaması ile Client 1 ve Client 2 rolleri çalıştırılabilir. Aynı istemciyi iki kez çalıştırarak (biri Sender, biri Receiver olacak şekilde) test yapılır.

---
## Hata Kontrolü (Protokol + Mesaj Seviyesi)

Projede mesajların bozulmaması için sadece `send/recv` dönüş değerleri değil,
**mesajın bütünlüğü** de kontrol edilir:

- TCP’de mesajlar parçalı gelebileceği için paket/parça biriktirme ve tam paket okuma mantığı uygulanır.
- Paket ayrıştırmada `split("|")` sonucu beklenen alan sayısı / method geçerliliği kontrol edilir.
- DATA/CONTROL alanları için uzunluk/format doğrulaması yapılarak “geçersiz paket” durumları yönetilir.

---
## Kurulum ve Çalıştırma (Visual Studio)

1. `ChatRoom.sln` dosyasını Visual Studio ile aç.
2. Önce **ChatServer** projesini çalıştır.
3. Ardından **ChatClient** uygulamasını iki farklı rolde çalıştır:
   - 1. çalıştırma: **Client 2 (Receiver/Checker)** rolü
   - 2. çalıştırma: **Client 1 (Sender)** rolü
4. IP/Port gibi ayarlar `server.cpp` ve `client.cpp` içinde tanımlıdır. (Gerekirse localhost için 127.0.0.1 kullanılabilir.)

---
## Repo Notu (Neleri paylaşmıyoruz?)

Bu repo sadece kaynak kodu içerir. Derleme çıktıları ve kullanıcıya özel dosyalar `.gitignore` ile hariç tutulmalıdır:
- `.vs/`, `x64/`, `Debug/Release`, `*.vcxproj.user`, `*.obj`, `*.pdb`, `logs/` vb.
