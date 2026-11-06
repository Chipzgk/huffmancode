#include<bits/stdc++.h>

using namespace std;


uint64_t cntFreq = 0;   //so lan tang tan suat
uint64_t cntCmp = 0;	//so phep so sanh trong heatmap
uint64_t cntSwap = 0;	//so lan hoan doi 2 nut
uint64_t cntMerge = 0;	//so lan ghep 2 nut trong cay huffman
uint64_t cntVisit = 0;	//so nut duoc dfs do
uint64_t cntBit = 0;	//so bit ghi/doc

// =============================== NUT CAY =================================

struct Node {
    uint64_t freq;			//tan suat cua nut
    int ch;					//gia tri byte neu la nut la
    Node *left, *right;		
};

Node* newNode(uint64_t f, int c, Node* l, Node* r) {
    return new Node{f, c, l, r};
}

// =============================== MIN HEAP ================================

struct MinHeap {
    Node* a[256];
    int size = 0;

	//doi 2 nut
    void swapNode(int i, int j) {
        Node* t = a[i];
        a[i] = a[j];
        a[j] = t;
        cntSwap++;
    }

	//day 1 nut len tren
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;

            cntCmp++;

            if (a[p]->freq <= a[i]->freq)
                break;

            swapNode(p, i);
            i = p;
        }
    }

	//day 1 nut xuong duoi
    void siftDown(int i) {
        while (true) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            int m = i;

            if (l < size) {
                cntCmp++;
                if (a[l]->freq < a[m]->freq)
                    m = l;
            }

            if (r < size) {
                cntCmp++;
                if (a[r]->freq < a[m]->freq)
                    m = r;
            }

            if (m == i)
                break;

            swapNode(i, m);
            i = m;
        }
    }

    void push(Node* x) {
        a[size] = x;
        size++;
        siftUp(size - 1);
    }

    Node* pop() {
        if (size == 0)
            return nullptr;

        Node* top = a[0];

        size--;

        if (size > 0) {
            a[0] = a[size];
            siftDown(0);
        }

        return top;
    }
};

// ============================ XAY CAY HUFFMAN ============================

Node* buildTree(const uint64_t freq[256]) {
    MinHeap h;

	//duyet 256 bit, tao la de dua vafo min-heap
    for (int c = 0; c < 256; c++) {
        if (freq[c] > 0)
            h.push(newNode(freq[c], c, nullptr, nullptr));
    }

    if (h.size == 0)
        return nullptr;

    if (h.size == 1) {
        Node* only = h.pop();
        return newNode(only->freq, -1, only, nullptr);
    }

    while (h.size > 1) {
        Node* x = h.pop();
        Node* y = h.pop();

        h.push(newNode(x->freq + y->freq, -1, x, y));

        cntMerge++;
    }

    return h.pop();
}

// ============================ SINH BANG MA ===============================

char path[257];

void buildCodes(Node* u, int depth, string code[256]) {
    if (u == nullptr)
        return;

    cntVisit++;

    if (u->ch != -1) {
        code[u->ch] = (depth == 0)
            ? "0"
            : string(path, depth);

        return;
    }

    path[depth] = '0';
    buildCodes(u->left, depth + 1, code);

    path[depth] = '1';
    buildCodes(u->right, depth + 1, code);
}

// ============================ GIAI PHONG CAY =============================

void freeTree(Node* u) {
    if (u == nullptr)
        return;

    freeTree(u->left);
    freeTree(u->right);

    delete u;
}

// ================================ GHI BIT ================================

struct BitWriter {
    ofstream& f;

    unsigned char buf = 0;
    int nbit = 0;

    BitWriter(ofstream& out) : f(out) {}

    void write(int b) {
        buf = (unsigned char)((buf << 1) | b);

        nbit++;
        cntBit++;

        if (nbit == 8) {
            f.put((char)buf);
            buf = 0;
            nbit = 0;
        }
    }

    void flush() {
        if (nbit > 0)
            f.put((char)(buf << (8 - nbit)));
    }
};

// ================================ DOC FILE ===============================

vector<unsigned char> readFile(const string& path) {
    ifstream f(path, ios::binary);

    if (!f)
        throw runtime_error("Khong mo duoc tep: " + path);

    return vector<unsigned char>(
        istreambuf_iterator<char>(f),
        istreambuf_iterator<char>()
    );
}

// =============================== DO THOI GIAN ============================

double msSince(chrono::steady_clock::time_point t0) {
    return chrono::duration<double, milli>(
        chrono::steady_clock::now() - t0
    ).count();
}

// ================================= NEN ====================================

void compress(const string& in, const string& out) {
    auto t0 = chrono::steady_clock::now();

    vector<unsigned char> data = readFile(in);
    uint64_t n = data.size();

    // Buoc 1: Dem tan suat
    uint64_t freq[256] = {0};

    for (uint64_t i = 0; i < n; i++) {
        freq[data[i]]++;
        cntFreq++;
    }

    int k = 0;

    for (int c = 0; c < 256; c++) {
        if (freq[c] > 0)
            k++;
    }

    // Buoc 2: Xay cay Huffman
    Node* root = buildTree(freq);

    // Buoc 3: Sinh bang ma
    string code[256];
    buildCodes(root, 0, code);

    // Buoc 4: Ghi header va ma hoa
    ofstream f(out, ios::binary);

    if (!f)
        throw runtime_error("Khong tao duoc tep nen");

    f.write(reinterpret_cast<const char*>(&n), sizeof(n));

    f.write(
        reinterpret_cast<const char*>(freq),
        sizeof(uint64_t) * 256
    );

    BitWriter bw(f);

    for (uint64_t i = 0; i < n; i++) {
        const string& s = code[data[i]];

        for (char b : s)
            bw.write(b - '0');
    }

    bw.flush();
    f.close();

    double ms = msSince(t0);

    // ========================== THONG KE ==========================

    double H = 0;

    for (int c = 0; c < 256; c++) {
        if (freq[c] > 0) {
            double p = (double)freq[c] / n;
            H -= p * log2(p);
        }
    }

    int dmax = 0;

    for (int c = 0; c < 256; c++) {
        if (freq[c] > 0)
            dmax = max(dmax, (int)code[c].size());
    }

    uint64_t dataBytes = (cntBit + 7) / 8;
    uint64_t outSize = 8 + 2048 + dataBytes;

    cout << "\n========== NEN FILE HUFFMAN ==========\n";

    cout << "So byte goc             : " << n << '\n';
    cout << "So byte phan biet       : " << k << '\n';

    cout << "\n--- THONG KE THUAT TOAN ---\n";

    cout << "So lan dem tan suat     : " << cntFreq << '\n';
    cout << "So lan gop nut          : " << cntMerge << '\n';
    cout << "So phep so sanh heap    : " << cntCmp << '\n';
    cout << "So phep hoan doi heap   : " << cntSwap << '\n';
    cout << "So nut tham DFS         : " << cntVisit << '\n';
    cout << "So bit ghi              : " << cntBit << '\n';

    cout << "\n--- THONG SO HUFFMAN ---\n";

    cout << fixed << setprecision(4);

    cout << "Do sau cay lon nhat     : " << dmax << '\n';
    cout << "Entropy                 : " << H << " bit/byte\n";

    cout << "Do dai ma trung binh    : "
         << (n ? (double)cntBit / n : 0.0)
         << " bit/byte\n";

    cout << "\n--- KET QUA NEN ---\n";

    cout << "Kich thuoc header       : 2056 byte\n";
    cout << "Kich thuoc du lieu nen  : " << dataBytes << " byte\n";
    cout << "Tong kich thuoc         : " << outSize << " byte\n";

    cout << fixed << setprecision(2);

    cout << "Ty le kich thuoc        : "
         << (n ? 100.0 * outSize / n : 0.0)
         << "%\n";

    cout << "Thoi gian nen           : " << ms << " ms\n";

    cout << "File dau ra             : " << out << '\n';

    cout << "======================================\n";

    freeTree(root);
}

// ================================ GIAI NEN ===============================

void decompress(const string& in, const string& out) {
    auto t0 = chrono::steady_clock::now();

    vector<unsigned char> data = readFile(in);

    if (data.size() < 2056)
        throw runtime_error("Tep nen khong hop le");

    uint64_t n;
    uint64_t freq[256];

    memcpy(&n, data.data(), 8);
    memcpy(freq, data.data() + 8, 2048);

    Node* root = buildTree(freq);

    ofstream f(out, ios::binary);

    if (!f) {
        freeTree(root);
        throw runtime_error("Khong tao duoc tep giai nen");
    }

    uint64_t written = 0;

    // Xu ly file rong
    if (n == 0) {
        f.close();
    }

    // Xu ly truong hop chi co mot loai byte
    else if (root && root->left && !root->right) {
        unsigned char ch = (unsigned char)root->left->ch;

        for (uint64_t i = 0; i < n; i++)
            f.put((char)ch);

        written = n;
    }

    // Xu ly truong hop thong thuong
    else {
        Node* cur = root;

        for (size_t i = 2056;
             i < data.size() && written < n;
             i++) {

            for (int b = 7; b >= 0 && written < n; b--) {
                int bit = (data[i] >> b) & 1;

                cntBit++;

                cur = bit ? cur->right : cur->left;

                if (cur == nullptr)
                    throw runtime_error("Du lieu nen khong hop le");

                if (cur->ch != -1) {
                    f.put((char)cur->ch);

                    written++;

                    cur = root;
                }
            }
        }
    }

    f.close();

    if (written != n && n != 0 && root &&
        root->left && !root->right) {
        // Truong hop mot loai byte da duoc khoi phuc o tren.
    }
    else if (written != n) {
        freeTree(root);
        throw runtime_error("Du lieu nen bi thieu hoac khong hop le");
    }

    cout << "\n======= GIAI NEN FILE HUFFMAN =======\n";

    cout << "So bit doc              : " << cntBit << '\n';
    cout << "So byte khoi phuc       : " << written << '\n';
    cout << "File dau ra             : " << out << '\n';

    cout << fixed << setprecision(2);

    cout << "Thoi gian giai nen      : "
         << msSince(t0) << " ms\n";

    cout << "====================================\n";

    freeTree(root);
}

// ================================= MAIN ===================================


int main() {
    int choice;
    string inputPath, outputPath;

    cout << "========== HUFFMAN COMPRESSOR ==========\n";
    cout << "1. Nen file\n";
    cout << "2. Giai nen file\n";
    cout << "========================================\n";

    cout << "Chon chuc nang: ";
    cin >> choice;

    if (choice != 1 && choice != 2) {
        cout << "Lua chon khong hop le!\n";
        return 1;
    }

    cout << "Nhap duong dan file dau vao: ";
    cin >> ws;
    getline(cin, inputPath);

    cout << "Nhap duong dan file dau ra: ";
    getline(cin, outputPath);

    try {
        if (choice == 1) {
            compress(inputPath, outputPath);
        }
        else {
            decompress(inputPath, outputPath);
        }
    }
    catch (const exception& e) {
        cout << "Loi: " << e.what() << '\n';
        return 1;
    }

    return 0;
}


