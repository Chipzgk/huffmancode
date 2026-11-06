# BÀI KIỂM TRA GIỮA KỲ – THUẬT TOÁN NÂNG CAO

**Câu 1:** Vận dụng Huffman Codes và Data Compression để nén dữ liệu.
**Loại dữ liệu chọn:** tệp âm thanh **WAV** (PCM 16-bit, chưa nén).
**Ngôn ngữ:** C++17 (`using namespace std`).

## Mã nguồn

| Tệp | Mô tả |
|---|---|
| `huffman_manual.cpp` | **Bản tự xây dựng**: tự cài đặt min-heap (`siftUp`, `siftDown`), cây Huffman, sinh mã DFS, bộ ghi bit. Có bộ đếm phép toán để kiểm chứng câu b. |
| `huffman_lib.cpp` | **Bản dùng thư viện STL**: dùng `priority_queue` làm min-heap, `array`, `string`, `vector`. Cùng thuật toán, cùng bộ đếm. |
| `sample.wav` | Tệp âm thanh nhỏ để chạy lại nhanh: 5 giây, mono, 44.100 Hz, 16-bit (441.044 byte). |

**Dữ liệu thực nghiệm chính:** một tệp nhạc WAV dài hơn 1 tiếng, kích thước **1.333.599.120 byte (≈ 1,24 GB)**. Tệp này vượt giới hạn 100 MB mỗi tệp của GitHub nên không có trong repo. Repo kèm `sample.wav` để người đọc tự chạy lại chương trình.

```bash
g++ -std=c++17 -O2 huffman_manual.cpp -o huffman_manual
g++ -std=c++17 -O2 huffman_lib.cpp    -o huffman_lib

huffman_manual c sample.wav sample.huf            # nén
huffman_manual d sample.huf sample_restored.wav   # giải nén
fc /b sample.wav sample_restored.wav              # Windows: kiểm tra giống hệt
cmp sample.wav sample_restored.wav                # Linux/macOS
```

> Mỗi bản giải nén tệp `.huf` do chính nó tạo ra. Khi hai nút có tần suất bằng nhau, hai loại heap có thể chọn khác nhau, nên cây có thể khác nhau. Cả hai cây vẫn tối ưu và cho cùng tổng số bit.

---

# Câu a. Trình bày thuật toán

## a.1. Thuật toán Huffman là gì?

**Định nghĩa.** Thuật toán Huffman (Huffman Coding) là thuật toán **nén dữ liệu không mất mát** (lossless). Thuật toán xây dựng một **mã tiền tố tối ưu** cho một tập ký hiệu dựa trên **tần suất xuất hiện** của từng ký hiệu: ký hiệu xuất hiện **càng nhiều** thì được gán mã nhị phân **càng ngắn**, ký hiệu xuất hiện **càng ít** thì mã **càng dài**.

**Nguồn gốc.** Thuật toán do **David A. Huffman** đề xuất năm **1952**, khi ông là nghiên cứu sinh tại MIT, trong bài báo *"A Method for the Construction of Minimum-Redundancy Codes"*.

**Phân loại.**
- Theo kỹ thuật thiết kế: là **thuật toán tham lam (greedy)**. Ở mỗi bước, thuật toán chọn phương án tốt nhất tại thời điểm đó (gộp hai nút có tần suất nhỏ nhất) và không quay lại sửa.
- Theo lĩnh vực: là **mã hóa entropy (entropy coding)**, tức là mã hóa dựa trên xác suất của ký hiệu.
- Theo kiểu mã: là **mã có độ dài thay đổi (variable-length code)**, khác với mã có độ dài cố định như ASCII (8 bit cho mọi ký tự).

**Ứng dụng thực tế.** Huffman là thành phần của nhiều chuẩn nén phổ biến: ZIP/gzip (DEFLATE = LZ77 + Huffman), PNG, JPEG, MP3, HTTP/2 (HPACK).

## a.2. Các khái niệm nền tảng

### Mã độ dài cố định và mã độ dài thay đổi

Cho bảng chữ cái $C$ gồm $k$ ký hiệu.

- **Mã độ dài cố định** dùng $\lceil \log_2 k \rceil$ bit cho **mọi** ký hiệu. Với byte ($k = 256$) là 8 bit mỗi ký hiệu, dù ký hiệu đó xuất hiện nhiều hay ít.
- **Mã độ dài thay đổi** cho phép mỗi ký hiệu có độ dài mã khác nhau. Nếu ký hiệu hay gặp có mã ngắn thì **tổng số bit giảm**. Đây là ý tưởng của Huffman.

Ví dụ với 100 ký hiệu (dùng lại ở mục a.10 và a.11):

| Ký hiệu | A | B | C | D | E | F | Tổng bit |
|---|---|---|---|---|---|---|---|
| Tần suất | 5 | 9 | 12 | 13 | 16 | 45 | |
| Mã cố định (3 bit) | 000 | 001 | 010 | 011 | 100 | 101 | 100 × 3 = **300** |
| Mã thay đổi (Huffman) | 1100 | 1101 | 100 | 101 | 111 | 0 | **224** |

### Mã tiền tố (prefix code)

**Định nghĩa.** Một bộ mã là **mã tiền tố** nếu **không có mã nào là phần đầu (tiền tố) của một mã khác**.

**Vì sao cần mã tiền tố?** Các mã được ghi liền nhau, không có dấu phân cách. Nếu không phải mã tiền tố thì giải mã bị nhập nhằng. Ví dụ với bộ mã `A = 0`, `B = 01`, `C = 1`, chuỗi `01` có thể là `B` hoặc `AC`. Với mã tiền tố, đọc bit từ trái sang phải, khớp đúng một mã là tách được ngay, chỉ có **một cách giải mã duy nhất**.

### Cây mã nhị phân

Mọi mã tiền tố đều biểu diễn được bằng một **cây nhị phân**:
- Mỗi **lá** là một ký hiệu.
- Cạnh đi sang **con trái** mang bit `0`, cạnh sang **con phải** mang bit `1`.
- **Mã** của ký hiệu $c$ là dãy bit trên đường đi từ gốc đến lá $c$.
- **Độ dài mã** của $c$ bằng **độ sâu** $d_T(c)$ của lá $c$ trong cây $T$.

Vì ký hiệu chỉ nằm ở lá, đường đi tới một lá không thể là phần đầu của đường đi tới lá khác, nên cây luôn cho mã tiền tố.

Cây mã tối ưu luôn là **cây nhị phân đầy đủ**: mọi nút trong đều có đúng 2 con. Nếu một nút trong chỉ có 1 con thì có thể bỏ nút đó để rút ngắn mã. Cây nhị phân đầy đủ có $k$ lá thì có $k - 1$ nút trong, tổng cộng $2k - 1$ nút.

## a.3. Công thức của thuật toán

### Bài toán tối ưu

Cho bảng chữ cái $C$, mỗi ký hiệu $c \in C$ có tần suất $f(c)$. Tìm cây mã $T$ sao cho **tổng số bit** sau khi mã hóa là **nhỏ nhất**:

$$\boxed{B(T) = \sum_{c \in C} f(c) \cdot d_T(c) \;\longrightarrow\; \min}$$

- $f(c)$: số lần ký hiệu $c$ xuất hiện.
- $d_T(c)$: độ sâu của lá $c$ trong cây $T$, cũng là độ dài mã của $c$.
- $B(T)$: gọi là **chi phí** của cây $T$.

### Độ dài mã trung bình

$$L = \frac{B(T)}{n} = \sum_{c \in C} p(c) \cdot d_T(c), \qquad p(c) = \frac{f(c)}{n}, \qquad n = \sum_{c \in C} f(c)$$

$L$ là số bit trung bình dùng cho mỗi ký hiệu. Mã cố định 8 bit có $L = 8$.

### Công thức gộp nút

Khi gộp hai nút $x$ và $y$ thành nút cha $z$:

$$f(z) = f(x) + f(y)$$

Mỗi lần gộp, mọi lá thuộc cây con $x$ và $y$ đều sâu thêm 1 tầng, nên chi phí tăng thêm đúng $f(x) + f(y)$. Do đó chi phí của cây cuối cùng bằng **tổng tần suất của tất cả các nút trong**:

$$B(T) = \sum_{z \text{ là nút trong}} f(z)$$

Kiểm tra với ví dụ ở a.2: các nút trong có tần suất 14, 25, 30, 55, 100, và $14 + 25 + 30 + 55 + 100 = 224$, đúng bằng tổng bit đã tính.

Công thức này giải thích vì sao **chiến lược tham lam** hợp lý: muốn tổng các nút trong nhỏ nhất thì mỗi bước nên gộp hai nút **nhỏ nhất**.

### Bất đẳng thức Kraft

Một bộ độ dài mã $\ell_1, \ell_2, \dots, \ell_k$ là độ dài của một mã tiền tố khi và chỉ khi:

$$\sum_{i=1}^{k} 2^{-\ell_i} \le 1$$

Với cây nhị phân đầy đủ (như cây Huffman), dấu bằng xảy ra: $\sum 2^{-\ell_i} = 1$. Trong ví dụ trên: $2^{-1} + 3 \cdot 2^{-3} + 2 \cdot 2^{-4} = 0{,}5 + 0{,}375 + 0{,}125 = 1$.

### Entropy Shannon và giới hạn của nén

$$H = -\sum_{c \in C} p(c) \log_2 p(c) \quad \text{(bit/ký hiệu)}$$

**Định lý mã hóa nguồn của Shannon:** mọi mã tiền tố đều có $L \ge H$. Mã Huffman đạt:

$$\boxed{H \le L_{\text{Huffman}} < H + 1}$$

Nghĩa là Huffman cách giới hạn lý thuyết **chưa tới 1 bit** mỗi ký hiệu. Hệ quả:
- Dữ liệu càng "lệch" (vài ký hiệu chiếm áp đảo) thì $H$ càng nhỏ và nén càng tốt.
- Dữ liệu phân bố đều ($p(c) = 1/256$ với mọi byte) thì $H = \log_2 256 = 8$, không nén được.

## a.4. Ý tưởng và các bước của thuật toán

**Ý tưởng:** xây cây **từ dưới lên**. Ban đầu mỗi ký hiệu là một cây chỉ có 1 nút. Mỗi bước, chọn **hai cây có tần suất nhỏ nhất** và gộp thành một cây mới có tần suất bằng tổng hai cây. Lặp lại cho đến khi còn một cây duy nhất. Hai ký hiệu ít gặp nhất bị gộp sớm nhất, nên nằm sâu nhất trong cây và có mã dài nhất.

**Thuật toán Huffman tổng quát** (theo Cormen et al., *Introduction to Algorithms*, mục 16.3):

```
HUFFMAN(C)
1   n ← |C|
2   Q ← C                              // hàng đợi ưu tiên min theo f
3   for i ← 1 to n − 1
4       cấp phát nút mới z
5       z.left  ← x ← EXTRACT-MIN(Q)
6       z.right ← y ← EXTRACT-MIN(Q)
7       z.freq  ← x.freq + y.freq      // f(z) = f(x) + f(y)
8       INSERT(Q, z)
9   return EXTRACT-MIN(Q)              // gốc của cây
```

**Các bước khi áp dụng để nén một tệp:**

| Bước | Việc làm | Kết quả |
|---|---|---|
| 1 | Đếm tần suất $f(c)$ của từng ký hiệu | bảng tần suất |
| 2 | Chạy HUFFMAN(C) để xây cây | cây Huffman |
| 3 | Duyệt cây để lấy mã của từng ký hiệu | bảng mã |
| 4 | Thay mỗi ký hiệu bằng mã của nó, ghi ra tệp kèm bảng tần suất | tệp nén |
| Giải nén | Dựng lại cây từ bảng tần suất, đọc từng bit và đi trên cây | tệp gốc |

**Tính đúng đắn (tóm tắt).** Huffman cho cây tối ưu vì thỏa hai tính chất (chứng minh chi tiết ở c.4):
1. **Lựa chọn tham lam:** luôn có một cây tối ưu trong đó hai ký hiệu ít gặp nhất là hai lá anh em ở tầng sâu nhất.
2. **Cấu trúc con tối ưu:** sau khi gộp $x, y$ thành $z$, bài toán còn lại có cùng dạng và $B(T) = B(T') + f(x) + f(y)$.

Các mục a.5 đến a.14 dưới đây trình bày cách áp dụng thuật toán cho tệp âm thanh WAV và cách cài đặt chi tiết trong code.

## a.5. Bài toán áp dụng: nén tệp WAV

**Đầu vào:** tệp âm thanh WAV, đọc ở chế độ nhị phân thành dãy $n$ byte $x_1, x_2, \dots, x_n$, với $x_i \in \lbrace 0, 1, \dots, 255\rbrace$.

Tệp WAV gồm 44 byte header, sau đó là các mẫu âm thanh. Mỗi mẫu 16-bit gồm 2 byte (byte thấp trước, byte cao sau). Chương trình coi **mỗi byte là một ký hiệu**, nên bảng chữ cái có tối đa 256 ký hiệu. Header được nén chung với dữ liệu, nhờ đó tệp giải nén ra là tệp WAV hoàn chỉnh, phát được ngay.

**Đầu ra:** tệp nén `.huf` nhỏ hơn, giải nén ra **giống hệt từng byte** với tệp gốc (nén không mất dữ liệu).

**Áp dụng công thức ở a.3:** bảng chữ cái $C$ là tập các giá trị byte xuất hiện trong tệp ($|C| = k \le 256$), và $f(c)$ là số lần byte $c$ xuất hiện. Mã cố định dùng 8 bit cho mọi byte. Huffman gán **mã ngắn cho byte hay gặp** và **mã dài cho byte hiếm**, nên tổng số bit $B(T)$ giảm.

## a.6. Các ký hiệu và công thức dùng trong code

| Ký hiệu | Ý nghĩa | Trong code |
|---|---|---|
| $n$ | số byte của tệp | `n = data.size()` |
| $f(c)$ | tần suất của byte $c$ | `freq[c]` |
| $k$ | số byte phân biệt (có $f(c) > 0$), $k \le 256$ | `k` |
| $d(c)$ | độ sâu lá $c$ trong cây = độ dài mã của $c$ | `code[c].size()` |
| $L$ | độ dài mã trung bình (bit/byte) | `cntBit / n` |
| $H$ | entropy (bit/byte) | `H` |

Các công thức chương trình sử dụng:

**(CT1) Tần suất:**
$$f(c) = \big|\lbrace \, i : x_i = c \,\rbrace \big|, \qquad \sum_{c} f(c) = n$$
Code: `freq[data[i]]++` với `i = 0..n-1`.

**(CT2) Gộp nút:**
$$f(z) = f(x) + f(y)$$
Code: `newNode(x->freq + y->freq, -1, x, y)`.

**(CT3) Chỉ số trong mảng heap**, với nút ở vị trí $i$:
$$\text{cha}(i) = \left\lfloor \frac{i-1}{2} \right\rfloor,\quad \text{trái}(i) = 2i+1,\quad \text{phải}(i) = 2i+2$$
Tính chất min-heap: $f(a[\text{cha}(i)]) \le f(a[i])$.
Code: `p = (i - 1) / 2`, `l = 2 * i + 1`, `r = 2 * i + 2`.

**(CT4) Tổng số bit sau mã hóa** (đại lượng Huffman làm nhỏ nhất):
$$B = \sum_{c} f(c) \cdot d(c)$$
Code: `cntBit` tăng 1 mỗi lần ghi một bit.

**(CT5) Độ dài mã trung bình:**
$$L = \frac{B}{n}$$

**(CT6) Entropy Shannon** (giới hạn dưới của $L$):
$$H = -\sum_{c} \frac{f(c)}{n} \log_2 \frac{f(c)}{n}$$
Code: `H -= p * log2(p)`.

**(CT7) Kích thước tệp nén:**
$$S = \underbrace{8}_{n} + \underbrace{256 \times 8}_{\text{freq}} + \left\lceil \frac{B}{8} \right\rceil = 2056 + \left\lceil \frac{B}{8} \right\rceil \text{ byte}$$
Code: `outSize = 8 + 2048 + (cntBit + 7) / 8`.

## a.7. Sơ đồ tổng quát

```
NÉN:
 sample.wav ─► [B1 Đếm tần suất] ─► freq[256]
                     │
                     ▼
              [B2 Xây cây Huffman bằng min-heap] ─► root
                     │
                     ▼
              [B3 Sinh bảng mã bằng DFS] ─► code[256]
                     │
                     ▼
              [B4 Ghi header + chuỗi bit] ─► sample.huf

GIẢI NÉN:
 sample.huf ─► [Đọc header] ─► [Dựng lại cây (B2)] ─► [Đọc bit, đi trên cây] ─► sample_restored.wav
```

Cấu trúc tệp `.huf`:

```
┌────────────┬────────────────────────────┬──────────────────────────┐
│ n (8 byte) │ freq[0..255] (2048 byte)   │ chuỗi bit (⌈B/8⌉ byte)   │
└────────────┴────────────────────────────┴──────────────────────────┘
```

## a.8. Bước 1 – Đếm tần suất

Duyệt toàn bộ $n$ byte, tăng `freq[x_i]` theo (CT1). Sau đó đếm $k$ = số giá trị có tần suất khác 0.

```cpp
uint64_t freq[256] = {0};
for (uint64_t i = 0; i < n; i++) freq[data[i]]++;
```

Mỗi byte được dùng trực tiếp làm chỉ số mảng, nên mỗi lần tăng chỉ tốn 1 phép toán.

## a.9. Bước 2 – Min-heap tự xây dựng

Huffman cần lặp lại thao tác **lấy 2 nút có tần suất nhỏ nhất**. Min-heap làm việc này nhanh: mỗi thao tác tốn $O(\log k)$, thay vì $O(k)$ nếu tìm tuần tự.

Heap được lưu trong mảng `a[0..size-1]` theo (CT3). Ví dụ một heap 6 phần tử:

```
 chỉ số:   0    1    2    3    4    5
 a[]   : [ 5 ][ 9 ][12 ][13 ][16 ][45 ]

              a[0]=5
             /      \
        a[1]=9      a[2]=12
        /    \       /
   a[3]=13 a[4]=16 a[5]=45
```

**siftUp(i)** đẩy phần tử mới lên đúng vị trí:
```
siftUp(i):
    while i > 0:
        p ← (i − 1) / 2                       // (CT3)
        if f(a[p]) ≤ f(a[i]): dừng            // 1 phép so sánh
        hoán đổi a[p], a[i];  i ← p
```
Mỗi vòng đi lên 1 tầng. Heap $m$ phần tử cao $\lfloor \log_2 m \rfloor$ tầng, nên siftUp tốn **tối đa $\lfloor \log_2 m \rfloor$ phép so sánh**.

**siftDown(i)** đẩy phần tử ở gốc xuống đúng vị trí:
```
siftDown(i):
    loop:
        l ← 2i + 1;  r ← 2i + 2;  m ← i        // (CT3)
        if l < size and f(a[l]) < f(a[m]): m ← l     // so sánh 1
        if r < size and f(a[r]) < f(a[m]): m ← r     // so sánh 2
        if m = i: dừng
        hoán đổi a[i], a[m];  i ← m
```
Mỗi tầng có 2 phép so sánh, nên siftDown tốn **tối đa $2\lfloor \log_2 m \rfloor$ phép so sánh**.

**push(x):** đặt `x` vào cuối mảng rồi gọi siftUp.
**pop():** lấy `a[0]`, chuyển phần tử cuối lên gốc rồi gọi siftDown.

## a.10. Bước 2 – Xây cây Huffman

```
BUILD-TREE(freq):
1   H ← heap rỗng
2   for c ← 0 to 255:
3       if freq[c] > 0: push(H, lá(c, freq[c]))      // k lần push
4   if |H| = 0: return NULL                           // tệp rỗng
5   if |H| = 1: return nút_cha(pop(H))                // chỉ 1 loại byte → mã "0"
6   while |H| > 1:                                    // đúng k − 1 vòng
7       x ← pop(H)                                    // nhỏ nhất
8       y ← pop(H)                                    // nhỏ nhì
9       z ← nút mới, z.left ← x, z.right ← y
10      f(z) ← f(x) + f(y)                            // (CT2)
11      push(H, z)
12  return pop(H)                                     // gốc cây
```

**Vì sao vòng lặp chạy đúng $k-1$ lần?** Mỗi vòng lấy ra 2 nút và đưa vào 1 nút, nên heap giảm đúng 1 phần tử. Từ $k$ xuống 1 cần $k-1$ vòng. Mỗi vòng tạo 1 nút trong, nên cây có $k$ lá + $(k-1)$ nút trong = **$2k-1$ nút**.

**Ví dụ minh họa.** Đoạn dữ liệu 100 byte có 6 giá trị:

| Byte | A | B | C | D | E | F |
|---|---|---|---|---|---|---|
| $f$ | 5 | 9 | 12 | 13 | 16 | 45 |

| Vòng | pop x, y | $f(z) = f(x)+f(y)$ | Heap sau vòng |
|---|---|---|---|
| 1 | A5, B9 | 14 | C12, D13, z14, E16, F45 |
| 2 | C12, D13 | 25 | z14, E16, z25, F45 |
| 3 | z14, E16 | 30 | z25, z30, F45 |
| 4 | z25, z30 | 55 | F45, z55 |
| 5 | F45, z55 | 100 | gốc 100 |

Có $k = 6$ nên chạy $k - 1 = 5$ vòng, cây có $2k-1 = 11$ nút.

```
                (100)
               0/   \1
             F45    (55)
                   0/   \1
                (25)     (30)
               0/  \1   0/   \1
             C12  D13 (14)   E16
                     0/  \1
                    A5    B9
```

## a.11. Bước 3 – Sinh bảng mã (DFS)

Đi từ gốc: rẽ trái ghi `0`, rẽ phải ghi `1`. Đến lá $c$ thì đường đi là `code[c]`, và độ dài mã $= d(c)$.

```
BUILD-CODES(u, depth):
1   if u = NULL: return
2   if u là lá: code[u.ch] ← path[0..depth−1]  (nếu depth = 0 thì "0"); return
3   path[depth] ← '0';  BUILD-CODES(u.left,  depth + 1)
4   path[depth] ← '1';  BUILD-CODES(u.right, depth + 1)
```

Mỗi nút được thăm đúng 1 lần, tổng cộng $2k-1$ lần thăm.

Kết quả cho ví dụ trên:

| Byte | F | C | D | E | A | B |
|---|---|---|---|---|---|---|
| Mã | `0` | `100` | `101` | `111` | `1100` | `1101` |
| $d(c)$ | 1 | 3 | 3 | 3 | 4 | 4 |

Áp dụng (CT4):
$$B = 45\cdot1 + 12\cdot3 + 13\cdot3 + 16\cdot3 + 5\cdot4 + 9\cdot4 = 224 \text{ bit}$$
$$L = 224/100 = 2{,}24 \text{ bit/byte} \quad (\text{so với 8 bit/byte: giảm } 72\%)$$

**Vì sao giải mã không bị nhầm?** Ký hiệu chỉ nằm ở lá, nên không mã nào là phần đầu của mã khác (gọi là **mã tiền tố**). Khi đọc bit và chạm tới lá, chắc chắn đã đọc trọn một mã.

## a.12. Bước 4 – Ghi header và mã hóa

```
COMPRESS(file):
1   ghi n (8 byte), ghi freq[0..255] (2048 byte)
2   buf ← 0; nbit ← 0
3   for i ← 1 to n:
4       for mỗi bit b trong code[x_i]:            // d(x_i) bit
5           buf ← (buf << 1) | b;  nbit ← nbit + 1
6           if nbit = 8: ghi buf; buf ← 0; nbit ← 0
7   if nbit > 0: ghi (buf << (8 − nbit))          // đệm 0 cho đủ byte cuối
```

- **Vì sao lưu `freq` trong header?** Để lúc giải nén dựng lại **đúng cây** đã dùng khi nén.
- **Vì sao lưu `n`?** Byte cuối có thể chứa bit 0 đệm thêm. Biết $n$ thì giải nén dừng đúng lúc, không sinh byte thừa.

## a.13. Giải nén

```
DECOMPRESS(file.huf):
1   đọc n, freq;  root ← BUILD-TREE(freq);  cur ← root;  đã_ghi ← 0
2   for mỗi bit b (từ trái sang phải), chừng nào đã_ghi < n:
3       cur ← (b = 0) ? cur.left : cur.right
4       if cur là lá:
5           ghi byte cur.ch;  đã_ghi ← đã_ghi + 1;  cur ← root
```

Mỗi bit đọc vào ứng với đi xuống 1 cạnh trên cây, nên tổng số bước giải nén bằng đúng $B$.

## a.14. Hai bản cài đặt

| Thành phần | `huffman_manual.cpp` (tự xây dựng) | `huffman_lib.cpp` (thư viện) |
|---|---|---|
| Min-heap | `struct MinHeap` tự viết: mảng `a[256]`, `siftUp`, `siftDown` theo (CT3) | `priority_queue<Node*, vector<Node*>, Cmp>` |
| Bảng mã | `string code[256]`, đường đi lưu trong mảng `char path[]` | `array<string,256>`, nối chuỗi `cur + '0'` |
| Ghi bit | `struct BitWriter` | ghi trực tiếp trong vòng lặp |
| Bộ đếm so sánh | đếm trong `siftUp`/`siftDown` | đếm trong `Cmp::operator()` |
| Header | `#include` từng thư viện cần dùng | `#include <bits/stdc++.h>` |

Cả hai dùng `using namespace std` và cùng các công thức CT1–CT7.

---

# Câu b. Tính toán các giá trị O, Ω, Θ

## b.1. Định nghĩa

| Ký hiệu | Định nghĩa | Ý nghĩa |
|---|---|---|
| $T(n) = O(g(n))$ | $\exists c>0, n_0: T(n) \le c\,g(n)\ \forall n \ge n_0$ | cận trên (trường hợp xấu nhất) |
| $T(n) = \Omega(g(n))$ | $\exists c>0, n_0: T(n) \ge c\,g(n)\ \forall n \ge n_0$ | cận dưới (trường hợp tốt nhất) |
| $T(n) = \Theta(g(n))$ | vừa $O(g)$ vừa $\Omega(g)$ | cận chặt |

## b.2. Số phép toán từng bước (từ công thức câu a)

### Bước 1 – Đếm tần suất

Vòng `for` chạy đúng $n$ lần, mỗi lần 1 phép `freq[x]++`:
$$T_1(n) = n$$
Trường hợp nào cũng phải đọc đủ $n$ byte, nên:
$$T_1 = O(n) = \Omega(n) = \Theta(n)$$

### Bước 2 – Xây cây (đếm số phép so sánh trong heap)

Theo a.9, push vào heap có $m$ phần tử tốn $\le \lfloor\log_2 m\rfloor$ so sánh, pop tốn $\le 2\lfloor\log_2 m\rfloor$ so sánh. Với $m \le k$:

- **Đưa $k$ lá vào heap** (dòng 2–3 của BUILD-TREE):
$$C_{\text{khởi tạo}} \le \sum_{m=1}^{k} \lfloor \log_2 m \rfloor \le k \log_2 k$$

- **$k-1$ vòng gộp**, mỗi vòng có 2 pop + 1 push:
$$C_{\text{gộp}} \le (k-1)\big(2\cdot 2\log_2 k + \log_2 k\big) = 5(k-1)\log_2 k$$

- **Tổng:**
$$C_2(k) \le k\log_2 k + 5(k-1)\log_2 k < 6k\log_2 k \;\Rightarrow\; T_2 = O(k \log k)$$

Cận dưới: thuật toán phải gộp đúng $k-1$ lần, và theo mô hình so sánh, Huffman có thể dùng để sắp xếp $k$ tần suất, mà sắp xếp cần $\Omega(k\log k)$ phép so sánh. Do đó $T_2 = \Omega(k \log k)$. Kết hợp lại:
$$T_2 = \Theta(k \log k)$$

### Bước 3 – Sinh mã

DFS thăm mỗi nút 1 lần, cây có $2k-1$ nút:
$$T_3(k) = 2k - 1 = \Theta(k)$$

### Bước 4 – Mã hóa

Mỗi byte $x_i$ ghi $d(x_i)$ bit. Theo (CT4) và (CT5):
$$T_4 = \sum_{i=1}^{n} d(x_i) = \sum_c f(c)\, d(c) = B = n \cdot L$$

Giá trị của $L$ phụ thuộc hình dạng cây:
- **Tốt nhất:** mọi mã dài ít nhất 1 bit, nên $L \ge 1$, suy ra $T_4 \ge n$, tức $T_4 = \Omega(n)$.
- **Xấu nhất:** độ sâu của cây $k$ lá không vượt quá $k-1$, nên $L \le k-1$, suy ra $T_4 \le n(k-1)$, tức $T_4 = O(nk)$.
- **Chính xác:** $T_4 = \Theta(n \cdot L)$.

### Giải nén

Dựng lại cây tốn $T_2$, đọc $B$ bit (mỗi bit đi 1 cạnh):
$$T_{\text{giải nén}} = \Theta(k \log k) + \Theta(n L)$$

## b.3. Tổng hợp

$$T_{\text{nén}}(n,k) = \underbrace{n}_{T_1} + \underbrace{\Theta(k\log k)}_{T_2} + \underbrace{(2k-1)}_{T_3} + \underbrace{nL}_{T_4} = \Theta(nL + k\log k)$$

| Bước | Công thức | O | Ω | Θ |
|---|---|---|---|---|
| B1 Đếm tần suất | $n$ | $O(n)$ | $\Omega(n)$ | $\Theta(n)$ |
| B2 Xây cây | $\le k\log_2 k + 5(k-1)\log_2 k$ | $O(k\log k)$ | $\Omega(k\log k)$ | $\Theta(k\log k)$ |
| B3 Sinh mã | $2k-1$ | $O(k)$ | $\Omega(k)$ | $\Theta(k)$ |
| B4 Mã hóa | $nL$ | $O(nk)$ | $\Omega(n)$ | $\Theta(nL)$ |
| Giải nén | $k\log k + nL$ | $O(nk)$ | $\Omega(n)$ | $\Theta(nL + k\log k)$ |
| **Tổng nén** | | $\boldsymbol{O(nk + k\log k)}$ | $\boldsymbol{\Omega(n + k\log k)}$ | $\boldsymbol{\Theta(nL + k\log k)}$ |

**Khi dữ liệu là byte:** $k \le 256$ là hằng số, nên $L \le 255$ và $k\log_2 k \le 2048$ cũng là hằng số. Ta chứng minh:

- **Cận trên:** $T(n) \le n + 6\cdot2048 + 511 + 255n \le 257n$ với mọi $n \ge 12800$, nên $T(n) = O(n)$.
- **Cận dưới:** $T(n) \ge T_1 = n$, nên $T(n) = \Omega(n)$.
- **Kết luận:** $\boxed{T(n) = \Theta(n)}$.

## b.4. Thay số với tệp nhạc WAV (hơn 1 tiếng)

Chạy `huffman_manual` với tệp nhạc có $n = 1.333.599.120$ byte và $k = 256$, nên $\log_2 k = 8$.

| Đại lượng | Công thức | Tính theo lý thuyết | Code đếm được |
|---|---|---|---|
| $T_1$ | $n$ | 1.333.599.120 | **1.333.599.120** |
| Số vòng gộp | $k-1$ | 255 | **255** |
| So sánh heap | $\le k\log_2k + 5(k-1)\log_2k$ | $\le 2048 + 10.200 = 12.248$ | **6.825** |
| Hoán đổi heap | $\le$ số so sánh | $\le 12.248$ | **3.530** |
| Số nút DFS | $2k-1$ | 511 | **511** |
| Độ sâu cây lớn nhất | $\le k-1$ | $\le 255$ | **9** |
| $H$ | (CT6) | — | **7,8210** bit/byte |
| $B = T_4$ | $\sum f(c)d(c)$ | — | **10.485.096.855** bit |
| $L$ | $B/n$ | $H \le L < H+1$, tức $7{,}8210 \le L < 8{,}8210$ | $\dfrac{10.485.096.855}{1.333.599.120}$ = **7,8623** ✓ |
| Dữ liệu nén | $\lceil B/8\rceil$ | $\lceil 1.310.637.106{,}9 \rceil$ = 1.310.637.107 | **1.310.637.107** byte |
| Kích thước nén $S$ | $2056 + \lceil B/8\rceil$ | 1.310.639.163 | **1.310.639.163** byte |
| Tỷ lệ $S/n$ | (CT7) | $\dfrac{1.310.639.163}{1.333.599.120}$ = 98,28% | **98,28%** |
| Số bit đọc khi giải nén | $B$ | 10.485.096.855 | **10.485.096.855** ✓ |
| Số byte khôi phục | $n$ | 1.333.599.120 | **1.333.599.120** ✓ |

Nhận xét:
- Các giá trị code đếm được **trùng khớp** với công thức: $T_1 = n$, đúng $k-1 = 255$ vòng gộp, đúng $2k-1 = 511$ nút. $S$ và tỷ lệ nén tính tay ra đúng bằng số chương trình in.
- Số phép so sánh thực tế (6.825) **nhỏ hơn cận trên** (12.248), vì không phải lần siftUp/siftDown nào cũng đi hết chiều cao heap.
- Độ sâu cây chỉ là 9, rất xa cận xấu nhất $k-1 = 255$. Lý do là các byte của nhạc phân bố khá đều, nên cây gần cân bằng ($\log_2 256 = 8$).
- So sánh heap gần như không đổi so với tệp `sample.wav` nhỏ (6.684), dù $n$ lớn gấp hơn 3.000 lần. Điều này đúng với công thức: $T_2$ chỉ phụ thuộc $k$, không phụ thuộc $n$.

Tổng số phép toán khi nén:
$$T \approx \underbrace{1.333.599.120}_{T_1} + \underbrace{6.825}_{T_2} + \underbrace{511}_{T_3} + \underbrace{10.485.096.855}_{T_4} \approx 11{,}82 \text{ tỷ}$$

Trong đó $T_1 + T_4$ chiếm **99,99994%**, còn phần xây cây $T_2 + T_3$ (7.336 phép toán) gần như không đáng kể. Vì vậy thời gian chạy phụ thuộc hoàn toàn vào $n$.

## b.5. Kiểm chứng trường hợp tốt nhất và xấu nhất

Mình chạy `huffman_manual` với 3 tệp đặc biệt:

| Tệp | Đặc điểm | $k$ | Độ sâu max | $L$ | Tỷ lệ nén | Trường hợp |
|---|---|---|---|---|---|---|
| WAV im lặng (441.044 byte, toàn mẫu 0) | 1 byte chiếm gần hết | 25 | 7 | **1,0003** | 12,97% | **Tốt nhất**: $L \to 1$, $T = \Omega(n)$ |
| Tần suất Fibonacci 1,1,2,3,5,… (25 giá trị) | cây lệch hẳn một phía | 25 | **24 = k−1** | 2,6179 | 33,77% | **Xấu nhất** về hình dạng cây: độ sâu đạt $k-1$ |
| Byte ngẫu nhiên đều | mọi byte như nhau | 256 | 8 | **8,0000** | 100,47% | Không nén được: $H = 8$ |

- **Tệp im lặng:** $L$ gần bằng 1, đúng cận dưới $T_4 \ge n$.
- **Tệp Fibonacci:** độ sâu lớn nhất đúng bằng $k-1 = 24$. Đây là chiều cao tối đa có thể có, ứng với cận $L \le k-1$.
- **Tệp ngẫu nhiên:** $L = H = 8$ nên Huffman không giảm được kích thước. Tệp nén còn lớn hơn tệp gốc 0,47% do header 2.056 byte.

---

# Câu c. Đánh giá độ phức tạp thuật toán

## c.1. Độ phức tạp thời gian

| | Tổng quát | Dữ liệu byte ($k \le 256$) |
|---|---|---|
| Nén | $\Theta(nL + k\log k)$ | $\Theta(n)$ |
| Giải nén | $\Theta(nL + k\log k)$ | $\Theta(n)$ |

**Thời gian với tệp nhạc thật** ($n$ = 1.333.599.120 byte):

| | Thời gian | Mỗi byte | Tốc độ |
|---|---|---|---|
| Nén | 247.881,9 ms ≈ 4 phút 8 giây | ≈ 0,19 µs | ≈ 5,4 MB/s |
| Giải nén | 218.752,8 ms ≈ 3 phút 39 giây | ≈ 0,16 µs | ≈ 6,1 MB/s |

Giải nén đọc đúng $B$ = 10.485.096.855 bit và khôi phục đúng $n$ = 1.333.599.120 byte, khớp với công thức ở a.13. Với tệp lớn như vậy, thời gian đọc và ghi đĩa (khoảng 1,3 GB vào và 1,3 GB ra) chiếm phần đáng kể trong tổng thời gian.

**Kiểm chứng tính tuyến tính.** Nhân `sample.wav` lên nhiều lần rồi chạy `huffman_manual` trên cùng một máy:

| $n$ (byte) | $B$ (bit) | Thời gian nén | Thời gian giải nén |
|---|---|---|---|
| 441.044 | 3.302.780 | 9,6 ms | 10,9 ms |
| 882.044 | 6.605.236 | 18,4 ms | 22,2 ms |
| 1.764.044 | 13.210.148 | 36,9 ms | 45,0 ms |
| 3.528.044 | 26.419.974 | 74,5 ms | 92,7 ms |
| 7.056.044 | 52.839.624 | 153,0 ms | 195,3 ms |

Mỗi lần $n$ **gấp đôi** thì $B$ và thời gian chạy cũng **gấp đôi** (9,6 → 18,4 → 36,9 → 74,5 → 153 ms). Đây là dấu hiệu của độ phức tạp tuyến tính $\Theta(n)$. Nếu thuật toán là $\Theta(n^2)$, thời gian phải tăng gấp 4 mỗi lần.

Tệp nhạc 1,33 GB lớn gấp khoảng 3.024 lần `sample.wav`. Nếu thuật toán là $\Theta(n^2)$, thời gian sẽ tăng khoảng 9 triệu lần và không thể chạy xong trong vài phút. Thực tế chương trình chạy xong trong 4 phút, phù hợp với $\Theta(n)$. (Hai thí nghiệm chạy trên hai máy khác nhau nên không so sánh trực tiếp số mili giây được.)

Với tệp nhỏ, giải nén chậm hơn nén một chút, vì phải đi trên cây theo con trỏ từng bit, trong khi nén chỉ tra bảng mã. Với tệp nhạc 1,33 GB thì ngược lại: giải nén nhanh hơn nén khoảng 12%. Lý do là khi tệp lớn, thời gian đọc ghi đĩa chiếm phần lớn, mà bước nén phải duyệt dữ liệu hai lần (đếm tần suất rồi mới mã hóa), còn giải nén chỉ duyệt một lần. Cả hai vẫn cùng bậc $\Theta(n)$.

## c.2. Độ phức tạp không gian

| Thành phần | Kích thước | Bậc |
|---|---|---|
| `freq[256]` | 2.048 byte | $\Theta(1)$ |
| Heap `a[256]` | $\le k$ con trỏ | $\Theta(k)$ |
| Cây Huffman | $2k-1$ nút | $\Theta(k)$ |
| Bảng mã | $\sum_c d(c) \le k(k-1)$ ký tự | $O(k^2)$ |
| Ngăn xếp đệ quy DFS | độ sâu cây $\le k-1$ | $O(k)$ |
| Bộ đệm `data` (đọc cả tệp vào RAM) | $n$ byte (≈ 1,33 GB với tệp nhạc) | $\Theta(n)$ |
| **Tổng** | | $\boldsymbol{\Theta(n + k^2)} = \Theta(n)$ với byte |

Có thể giảm bộ nhớ phụ xuống $O(1)$ bằng cách đọc tệp hai lượt theo luồng (lượt 1 đếm tần suất, lượt 2 mã hóa). Đổi lại, chương trình phải đọc đĩa hai lần.

## c.3. So sánh bản tự xây dựng và bản thư viện

Chạy cả hai bản với `sample.wav`:

| Tiêu chí | `huffman_manual` | `huffman_lib` |
|---|---|---|
| Độ phức tạp lý thuyết | $\Theta(nL + k\log k)$ | $\Theta(nL + k\log k)$ |
| So sánh heap thực tế | 6.684 | 4.452 |
| Thời gian nén `sample.wav` | 9,6 ms | 9,1 ms |
| Kích thước nén | 414.904 byte | 414.904 byte |
| $L$ | 7,4885 | 7,4885 |

Hai bản có cùng độ phức tạp và cùng kết quả nén. `priority_queue` của STL ít phép so sánh hơn vì cài đặt `pop` tối ưu hơn: nó đẩy "lỗ" xuống đáy bằng 1 so sánh mỗi tầng rồi mới đẩy lên. Khác biệt này chỉ là hằng số, không đổi bậc. Vì phần xây cây chỉ chiếm 0,2% tổng thời gian, thời gian chạy hai bản gần như bằng nhau.

## c.4. Tính tối ưu của thuật toán

Huffman là **thuật toán tham lam**: ở mỗi bước luôn gộp 2 nút nhỏ nhất. Lựa chọn này đúng vì thỏa hai tính chất sau.

1. **Lựa chọn tham lam:** nếu $x, y$ là 2 byte có tần suất nhỏ nhất, luôn tồn tại một cây tối ưu trong đó $x, y$ là 2 lá anh em ở tầng sâu nhất. Chứng minh: đổi $x$ với lá sâu nhất $a$ thì $B$ thay đổi một lượng $\Delta = (f(a) - f(x))(d(x) - d(a)) \le 0$, tức là không làm cây tệ đi.
2. **Cấu trúc con tối ưu:** gọi $T'$ là cây sau khi thay $x, y$ bằng $z$. Khi đó $B(T) = B(T') + f(x) + f(y)$. Vì $f(x) + f(y)$ là hằng số, $T'$ tối ưu thì $T$ cũng tối ưu.

Do đó Huffman cho **mã tiền tố tối ưu**, và thỏa $H \le L < H + 1$.

Với tệp nhạc: $L - H = 7{,}8623 - 7{,}8210 = 0{,}0413$ bit/byte. Huffman chỉ kém giới hạn lý thuyết khoảng **0,5%**, và còn cách rất xa giới hạn trên $H + 1$.

## c.5. Đánh giá kết quả nén âm thanh

| Dữ liệu | $H$ | $L$ | Tỷ lệ nén |
|---|---|---|---|
| **Tệp nhạc WAV hơn 1 tiếng** | **7,82** | **7,86** | **98,28%** |
| `sample.wav` (âm sin + nhiễu) | 7,46 | 7,49 | 94,07% |
| WAV im lặng | 0,0014 | 1,0003 | 12,97% |
| Byte ngẫu nhiên | 8,00 | 8,00 | 100,47% |

**Vì sao tệp nhạc chỉ nén được khoảng 1,7%?** Tệp giảm từ 1.333.599.120 xuống 1.310.639.163 byte, tiết kiệm **22.959.957 byte (≈ 23 MB)**.
- Entropy của tệp nhạc là 7,82 bit/byte, rất gần mức tối đa 8 bit. Theo $L \ge H$, **không mã tiền tố nào** (dù là Huffman hay cách khác) mã hóa từng byte độc lập mà nén được tốt hơn khoảng $7{,}82/8 = 97{,}8\%$. Kết quả 98,28% đã sát giới hạn này.
- Nhạc thật có âm lượng lớn và biến đổi nhiều hơn `sample.wav`, nên các byte phân bố đều hơn. Vì thế entropy cao hơn (7,82 so với 7,46) và nén kém hơn.
- Byte thấp của mỗi mẫu 16-bit thay đổi liên tục, phân bố gần đều trên cả 256 giá trị. Byte cao tập trung hơn nhưng bị trộn chung với byte thấp, nên entropy vẫn cao.
- Huffman chỉ dựa vào **tần suất từng byte riêng lẻ**, không khai thác được việc mẫu âm thanh sau rất gần mẫu trước.

Kết luận: tỷ lệ nén thấp **không phải do cài đặt sai**. Nguyên nhân là bản chất dữ liệu: entropy theo byte của âm thanh rất cao. Huffman vẫn làm đúng phần việc của nó, đạt $L$ chỉ hơn $H$ 0,04 bit.

**Vì sao tệp im lặng vẫn tốn ít nhất 1 bit mỗi byte?** Huffman không thể gán mã ngắn hơn 1 bit. Vì vậy khi $H \approx 0$, $L$ vẫn bằng 1 (đây là lúc $L$ cách xa $H$ nhất).

## c.6. Ưu điểm và nhược điểm

**Ưu điểm**
- Không mất dữ liệu: tệp giải nén trùng 100% với tệp gốc (đã kiểm tra bằng `cmp`).
- Nhanh: thời gian tuyến tính $\Theta(n)$, nén được tệp 1,33 GB trong khoảng 4 phút.
- Tối ưu trong lớp mã tiền tố, $L$ sát $H$.
- Dễ cài đặt: chỉ cần heap và cây nhị phân.

**Nhược điểm**
- Phải đọc dữ liệu 2 lần (đếm tần suất, rồi mới mã hóa).
- Header cố định 2.056 byte, nên tệp nhỏ hoặc tệp ngẫu nhiên có thể bị **to ra**.
- Mỗi ký hiệu tốn ít nhất 1 bit.
- Không khai thác tương quan giữa các mẫu âm thanh, nên nén âm thanh kém.

**Hướng cải tiến**
- **Delta encoding:** lưu $s_i - s_{i-1}$ thay cho $s_i$ trước khi chạy Huffman (ý tưởng của FLAC).
- **Canonical Huffman:** chỉ lưu độ dài mã thay vì bảng tần suất, header giảm từ 2.056 xuống 256 byte.
- **Arithmetic coding:** không bị giới hạn 1 bit mỗi ký hiệu.

## c.7. Kết luận

| | Kết quả |
|---|---|
| Thời gian | $O(nk + k\log k)$, $\Omega(n + k\log k)$, $\Theta(nL + k\log k)$; với byte là $\Theta(n)$ |
| Không gian | $\Theta(n)$ |
| Tối ưu | $H \le L < H + 1$, đo được $7{,}8210 \le 7{,}8623 < 8{,}8210$ ($L - H = 0{,}0413$) |
| Nén tệp nhạc WAV | 1.333.599.120 → 1.310.639.163 byte (98,28%), nén 247,9 giây, giải nén 218,8 giây, khôi phục đủ 1.333.599.120 byte |
| Nén `sample.wav` | 441.044 → 414.904 byte (94,07%), giải nén trùng 100% |

---

## Tài liệu tham khảo

1. T. H. Cormen et al., *Introduction to Algorithms*, 3rd ed., MIT Press, 2009 – Ch. 6 (Heapsort), Ch. 16.3 (Huffman codes).
2. D. A. Huffman, "A Method for the Construction of Minimum-Redundancy Codes", *Proc. IRE*, 1952.
3. C. E. Shannon, "A Mathematical Theory of Communication", *Bell System Technical Journal*, 1948.

**Link GitHub:** `https://github.com/<username>/<repo>`
