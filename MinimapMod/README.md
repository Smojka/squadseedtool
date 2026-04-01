# Circular Minimap Mod – Arma Reforger

Ekranın orta-altında dairesel bir mini-harita gösterir. Harita, M tuşuna basıldığında açılan haritanın aynı uydu/terrain dokusunu kullanır; yollar, dağlar ve diğer arazi detayları görünür. Mini-harita yalnızca oyuncu oyuna (spawn) girdiğinde görünür, lobi veya ölüm ekranında gizlenir.

---

## Özellikler

| Özellik | Detay |
|---|---|
| Konum | Ekranın orta-altı |
| Şekil | Dairesel (200 px) |
| İçerik | Oyunun terrain/uydu dokusu (M-tuşu haritasıyla aynı) |
| Yönlendirme | Oyuncunun bakış yönü daima yukarı gösterir; harita döner |
| Görünürlük | Yalnızca spawn olmuş oyuncular görür |
| Koordinat | Haritanın altında küçük grid koordinatı gösterilir |

---

## Dosya Yapısı

```
MinimapMod/
├── addon.json                                      ← Mod meta verisi
├── README.md
├── Scripts/
│   └── Game/
│       └── UI/
│           └── Minimap/
│               ├── MM_MinimapDisplay.c             ← Ana HUD sınıfı
│               └── MM_MinimapHUDManager.c          ← HUD kayıt bileşeni
└── UI/
    └── layouts/
        └── MM_MinimapHUD.layout                    ← Widget ağacı (XML kaynak)
```

---

## Kurulum

### 1. Workbench'te Layout Dosyasını Hazırlama

Arma Reforger, `.layout` dosyalarını binary (ikili) formatta saklar. Depodaki
`UI/layouts/MM_MinimapHUD.layout` dosyası XML kaynak biçimindedir ve Workbench
ile binary formata dönüştürülmesi gerekir.

1. **Arma Reforger Workbench**'i açın.  
2. Mod klasörünüzü proje olarak ekleyin (`File → New Project → Existing Folder`).  
3. *Resource Manager* panelinde `UI/layouts/` klasörüne sağ tıklayıp  
   **Create → UI Layout** seçin ve `MM_MinimapHUD.layout` adını verin.  
4. **Layout Editor**'da açılan boş layout içine, depodaki XML dosyasında
   tanımlanan widget hiyerarşisini (Root → MinimapFrame → MinimapMap vb.)
   oluşturun.  Widget özelliklerini XML dosyasındaki değerlerle eşleştirin.  
5. Kaydedin; Workbench binary `.layout` dosyasını üretir.

### 2. Texture Varlıklarını Hazırlama

`UI/textures/` klasörü altına aşağıdaki `.edds` dosyalarını ekleyin
(Workbench'in Image Importer'ı ile PNG'lerinizi convert edin):

| Dosya | İçerik |
|---|---|
| `minimap_mask.edds` | Ortasında beyaz daire olan şeffaf PNG (200×200) |
| `minimap_border.edds` | Dairesel çerçeve/kenar halkası (210×210) |
| `player_arrow.edds` | Küçük üçgen/ok (oyuncu konumu, 16×16) |

### 3. Terrain Dokusunu Bağlama

`MM_MinimapDisplay.c` içinde `RefreshMapTexture()` metodu, `ImageWidget.SetUVRect()`
ile UV koordinatlarını güncelleyerek oyuncunun etrafındaki alanı gösterir.
Harita dokunuzu bağlamak için iki yöntem vardır:

**A) Workbench'te statik atama (önerilir)**  
Layout Editor'da `MinimapMap` widget'ına kaynak olarak dünya uydu dokusunu
seçin; örneğin:
```
worlds/Everon/data/everon_sat.edds
```

**B) Script ile dinamik yükleme**  
`MM_MinimapDisplay.OnStartDraw()` içine aşağıdaki satırları ekleyin:
```cpp
if (m_wMinimapMap)
{
    ResourceName satTex = "{DOKU_GUID}worlds/Everon/data/everon_sat.edds";
    m_wMinimapMap.LoadImageTexture(0, satTex);
}
```
GUID değerini Workbench Resource Manager'dan kopyalayın.

### 4. HUD Yöneticisi Bileşenini Ekleme

Oyuncu HUD'ına minimap'i dahil etmek için iki seçenek vardır:

**Seçenek A – PlayerController prefab'ına ekleyin:**  
1. Workbench'te oyun modunuzun PlayerController prefab'ını açın.  
2. `SCR_HUDManagerComponent` bileşenini bulun.  
3. `InfoDisplays` listesine `MM_MinimapHUD.layout`'u ekleyin.

**Seçenek B – Otomatik kayıt (MM_MinimapHUDManager bileşeni):**  
1. Workbench'te GameMode prefab'ınızı açın.  
2. `MM_MinimapHUDManager` bileşenini GameMode entity'ye ekleyin.  
Bileşen, oyun başladığında minimap'i otomatik olarak kaydeder.

### 5. Mod'u Aktif Etme

Sunucu `config.json` dosyanıza ekleyin:
```json
"mods": [
  {
    "modId": "5E4F2B8C9D1A3C7E",
    "name": "Circular Minimap",
    "version": "1.0.0"
  }
]
```

---

## Ayarlar

`MM_MinimapConfig` sınıfındaki [Attribute] değerlerini Workbench attribute
panelinden değiştirebilirsiniz:

| Özellik | Varsayılan | Açıklama |
|---|---|---|
| `m_fMinimapSize` | 200 px | Dairesel widget çapı |
| `m_fMapRadius` | 300 m | Haritada görünen dünya yarıçapı |
| `m_fBackgroundOpacity` | 0.85 | Arka plan saydamlığı (0–1) |
| `m_bRotateWithPlayer` | true | Haritanın oyuncuyla dönmesi |
| `m_fBottomMargin` | 20 px | Ekran altından piksel mesafesi |

---

## Teknik Notlar

- Minimap **yalnızca istemci tarafında** çalışır; sunucuya ekstra yük bindirmez.
- `SCR_InfoDisplay` alt sınıfı olduğu için Arma Reforger'ın standart HUD
  yaşam döngüsüyle (spawn/ölüm/respawn) tam uyumludur.
- `UpdateValues()` içindeki `UPDATE_INTERVAL = 0.05 s` değeri minimap'i 20 fps'de
  günceller; bu, UI için yeterli performans ile kaynak kullanımı arasında
  dengeli bir değerdir.
- UV hesaplaması kare dünya varsayımına dayanır. Dikdörtgen dünyalar için
  `CacheWorldSize()` metodunu `GetWorldSizeX()` / `GetWorldSizeZ()` kullanacak
  şekilde güncelleyin.

---

## Lisans

MIT – dilediğiniz gibi kullanabilir, değiştirebilir ve dağıtabilirsiniz.
