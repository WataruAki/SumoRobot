# Sumo Robot 5-Phase - ESP32 + TB6612 + TT Motor

## Cau hinh nguon

- Pin 2S cap truc tiep vao VM cua TB6612.
- Mot LM2596 chinh 5.0 V chi cap cho ESP32 va cam bien.
- Tat ca GND noi chung.
- Khong cap chung hai motor qua LM2596 trong ban thi dau.
- ECHO cua HC-SR04 phai qua cau chia ap 5 V xuong 3.3 V.

## Anh xa GPIO

| Chuc nang | GPIO |
|---|---:|
| PWMA / AIN1 / AIN2 | 18 / 21 / 19 |
| PWMB / BIN1 / BIN2 | 5 / 22 / 23 |
| HC-SR04 TRIG / ECHO | 25 / 26 |
| Nut START | 27 |
| Cam bien bien truoc | 32 |
| TB6612 STBY | Noi co dinh 3V3 |

Kenh A duoc xem la motor trai, kenh B la motor phai. Neu mot ben quay nguoc, doi
`INVERT_LEFT_MOTOR` hoac `INVERT_RIGHT_MOTOR` thanh `true`.

## Switch RUN/STOP

- OFF = GPIO27 HIGH: cat PWM ngay va reset may trang thai ve `WAIT_START`.
- ON = GPIO27 LOW on dinh 35 ms: bat dau lai pha 1 va dem du 5 giay.
- OFF khong chi la lenh START nua; no la kill switch phan mem trong moi pha.
- Truoc khi reset/cap nguon, de switch OFF. Sau hieu lenh moi gat ON.

Serial khi gat OFF trong luc dang chay:

```text
[SWITCH OFF] Da cat motor va reset ve WAIT_START.
```

## Nam pha chinh

1. `PHASE_1_PROBING`: dung yen 5 giay, thu mau sieu am va tinh trung vi.
2. `PHASE_2_JUKE`: ne trai/phai co phan hoi, can 3 lan mat muc tieu moi dung.
3. `PHASE_3_RELOCK`: xoay tim, doi huong sau 420 ms, timeout 900 ms.
4. `PHASE_4_STRIKE`: tiep can, thu tiep xuc 100 ms, burst 80 ms.
5. `PHASE_5_RING_CONTROL`: bam muc tieu, chong neo, danh suon va thoat bien.

## Adaptive Extreme (van chi 5 pha)

- Loc median 3 mau sieu am de giam xung nhay/phan xa gia.
- Uoc luong toc do ap sat theo cm/s tu cac mau raw lien tiep.
- Can 2 mau charge lien tiep moi kich hoat ne, tranh phan ung voi mot mau nhieu.
- Khi doi thu lao nhanh trong 16-45 cm, ne trai/phai 90-140 ms roi khoa lai.
- Co cooldown 650 ms de doi thu khong the dung dong tac gia lam robot ne lien tuc.
- Random co gioi han thoi gian juke, search, contact test, burst va flank arc.
- Cac gioi han PWM, timeout motor va uu tien cam bien bien khong thay doi.

Day la cac trang thai con nam trong pha 5, khong phai them pha thu 6.

## Cam bien line - quyen uu tien cao nhat

- GPIO32 dung `INPUT_PULLUP`, cam bien nen cap 3V3.
- Khi khoi dong, dat robot de cam bien nam tren phan SAN DEN.
- Code lay 80 mau de hoc muc san den va tu suy ra muc vach trang doi nghich.
- Neu tin hieu dao qua 6 lan trong luc calibration, code bao loi va dung muc fallback.
- Gap vach thi ngat chien thuat ngay lap tuc, khong debounce de tranh bo lo vach 2.5 cm.
- Chi roi trang thai thoat bien sau khi da roi vach on dinh 20 ms.

Serial khoi dong dung phai co dang:

```text
[LINE] OK: floor=1, white=0, transitions=0.
```

Neu robot khoi dong khi cam bien dang nam tren vach trang, calibration se bi dao va
robot co the xem san den la vach. Luon dat tren san den truoc khi cap nguon/reset.

## Chien thuat chong neo / quat hut

- Tiep can o PWM 175.
- Tai khoang cach tiep xuc, thu day 100 ms o PWM 190.
- Burst them 80 ms o PWM 205.
- Neu muc tieu van sat dau xe, khong ghe motor: lui 65 ms.
- Chay cung danh suon 220 ms, luan phien trai/phai.
- Sau cung danh suon, quay lai pha khoa muc tieu.

Khong co encoder/cam bien dong nen day la phan loai bao thu. Robot xem tinh huong
"muc tieu van sat sau 180 ms" la nguy co neo, khong phai bang chung chac chan.

## Gioi han PWM voi pin 2S

- Search: 90
- Juke: 190
- Approach: 175
- Contact test: 190
- Attack burst: 205 trong toi da 80 ms
- Escape: 160
- Flank arc: 75 / 185

PWM 255 chi duoc dung cho short-brake cua TB6612, khong dung de keo TT lien tuc.

## Serial debug

Mo Serial Monitor o 115200 baud. Code in trang thai moi 300 ms, khong in moi
vong loop de tranh lam cham robot.

Vi du:

```text
[DBG] t=8350 state=P3_RELOCK sub=- US=24cm seen/lost=2/0 close=85cm/s charge=1 edgeRaw/white=1/0 start=0 motorL/R=90/-90
```

- `state`: pha hien tai.
- `sub`: trang thai con cua pha 4/5.
- `US`: khoang cach sieu am gan nhat; `-1` la timeout.
- `seen/lost`: so mau thay/mat lien tiep.
- `close`: toc do ap sat uoc luong; so duong la dang tien gan.
- `charge`: 1 khi da xac nhan doi thu lao nhanh.
- `edgeRaw/white`: muc raw hien tai / muc da hoc cho vach trang.
- `start`: muc logic cong tac START; ON thuong la 0.
- `motorL/R`: lenh PWM co dau cua motor trai/phai.

Dat `DEBUG_SERIAL=false` sau khi hieu chinh xong neu muon tat log dinh ky.

## Hieu chinh truoc tran

1. Chinh LM2596 = 5.0 V truoc khi cam ESP32.
2. Kiem tra hai banh tien dung chieu; sua hai bien `INVERT_*` neu can.
3. Dat robot cach vach, xac dinh `EDGE_WHITE_LEVEL` la `HIGH` hay `LOW`.
4. Cho luoi xuc vua cham vat, doc Serial va sua `CONTACT_DISTANCE_CM`.
5. Thu edge reverse tu toc do thap; giam `EDGE_REVERSE_MAX_MS` neu lui qua xa.
6. Thu flank arc khi robot o giua san; giam `FLANK_ARC_MS` neu cung qua rong.
7. Kiem tra pin day 8.4 V; motor va TB6612 khong duoc nong nhanh.
8. Khi nap code qua USB, tranh dong thoi cap nguon 5 V nguoc vao cong USB neu mach
   chua co power-ORing.

## Trang thai xac minh

Da kiem tra cu phap voi Arduino-ESP32 Core 2.x va 3.x. Code chi dung HC-SR04 de
phat hien doi thu, khong can thu vien ToF/I2C. Van can bien dich tren dung board
package va thu tai thuc te truoc thi dau.
