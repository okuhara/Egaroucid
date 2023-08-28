# Egaroucid 技術解説

**ここに書いた内容は高度なものです。一般的なアルゴリズムの話はしません。オセロAI初心者向け記事は[こちら](https://note.com/nyanyan_cubetech/m/m54104c8d2f12)に書きました。**

このページは日本語のみで、のんびりと気が向いたときに書き足していきます。



## minimaxかMCTSか

ゲームAIを作る上でのアルゴリズムは古くからminimax法が有名でした。1997年にチェスで人間を打ち負かしたDeep Blueも、同年にオセロで人間を打ち負かしたLogistelloも、このminimax法から派生するアルゴリズムでした。しかし、2000年に入るとMCTS(モンテカルロ木探索)が発展し、さらに2010年代にはMCTSをさらにアップデートしたPV-MCTS (Policy Value MCTS)が発展しました。2016年に囲碁で人間を打ち負かしたAlphaGoはPV-MCTSです。

この2つのアルゴリズムは根本から違うものです。将棋AIでは聞くところによるとすでにPV-MCTS優勢(2022年現在の伝聞)だそうですが、オセロにおいてはどちらを使うのが良いでしょうか。

オセロにおいてminimax系統が良いのかMCTS系統が良いのかは、まだ断言できないと思います。ですが、私はEgaroucidに対してminimax法から派生したNegascout法を利用しました。この理由について、軽くまとめます。



### オセロでは精度の高い評価関数を簡単に作れるから

minimax法では、計算量の問題で終局まで読みきれない場合には終局前に評価関数を使って、それをそのまま終局まで読んだ確定値と同じように利用します。つまり、評価関数の精度が悪ければ必然的にAIは弱くなります。

囲碁では評価関数が作りにくいようで、そのために評価関数を使わなくて済むMCTSが発展したそうです。

オセロにおいては、Logistelloに関する論文「Experiments with Multi-ProbCut and a New High-Quality Evaluation Function for Othello」でパターンを用いた評価関数が提案されてから、(それなりに)手軽に高精度の評価関数が作れるようになりました。ですので、この方法を踏襲すれば良かったわけです。

### オセロは合法手数が比較的少ないから

MCTSは評価関数を必要としない以外にも利点があります。それは、合法手が多いゲームでもそれなりの性能が出せるところです。オセロは局面1つに対して概ね10手前後の合法手しかありませんが、例えば囲碁はとんでもないことになります。

各局面の合法手数を$b$、探索する深さを$d$とすると、minimax法の計算量は$b^d$、そしてminimax法に効果的な枝刈りを施したαβ法は$\sqrt{b^d}$です。これでは、合法手が多いゲームでは計算量が爆発しやすくなって困ります。

しかし、オセロに関して言えばこの問題は囲碁などのゲームと比較して大きくありません。

### 実験してみてあまり強くならなかったから

[CodinGame Othello](https://www.codingame.com/multiplayer/bot-programming/othello-1)というオセロAIのコンテストに参加しているときに、minimaxとMCTSを両方試してみたのですが、どうも私の実装ではMCTSがあまり強くなりませんでした。あくまでも私の実装力の範囲の話なので、本当にオセロにMCTSが向いていないのかを議論することはできませんが、とりあえずminimaxを使おうという結論になりました。

### 完全読みの実装に繋げたいから

Egaroucidでは終盤の決められた手数で完全読みをしてしまいます。完全読みはその名の通り、終局まで完璧に読み切ってしまうことですが、これはminimax系統のアルゴリズムで行います。Egaroucidでは完全読みの際、完全読み以前の探索結果を使うことで探索の高速化を実現しています。中盤探索をMCTSで作るとこのあたりの実装が多少煩雑になることが想像できます。



## 評価関数

既存の強豪オセロAIに広く使われているパターン評価は、Logistelloで提案され、今日のEdaxまでほとんど形を変えずに受け継がれています。Egaroucidもこのパターン評価をベースにしていますが、少し特徴量を追加しました。

Egaroucidでは石自体をパターンとして評価する既存手法に加え、合法手をパターン化したもの、追加の特徴量の全ての得点の和を評価値としました。

評価関数は2手を1つのフェーズとして合計30フェーズ用意し、それぞれ大量の学習データを用意し、確率的勾配降下法で最適化しました。

### 石評価パターン

盤面のパターンとして使ったのは以下です。

<div class="centering_box">
    <img class="pic2" src="img/pattern.png">
</div>

### 合法手評価パターン

Egaroucidでは独自の評価パターンとして、合法手のパターンを用いています。両手番の合法手か否かの情報を1マスにつき2bitで割り当て、8マスをセットにしてパターンとしています。

<div class="centering_box">
    <img class="pic2" src="img/legal_pattern.png">
</div>

### その他の特徴量

Egaroucidではさらに、それぞれの手番について以下の項目を計算し、特徴量に追加しました。

<ul>
    <li>石が空きマスに接している数</li>
    <li>合法手数</li>
    <li>石数</li>
</ul>


## ボードの実装

Egaroucidではボードの実装およびオセロのルールの実装にビットボードを使用しています。オセロは8x8の64マスの盤面を使うため、64bit整数2つを使い、自分の石のありなしを表すのに64bit、相手の石のありなしを表すのに64bit使うと収まりが良いです。

合法手生成や返る石の計算などをすべてビット演算で完結できるため、高速に動きます。



## 枝刈り

Egaroucidでは、様々な方法で探索を省略(枝刈り)しています。特に込み入った枝刈りはEdaxに実装されているものを参考にしています。

### Negascout法

Egaroucidではminimax法系統の探索をしているので、Negascout法によって枝刈りを増やしています。Negascout法はαβ法に加え、最善(と思しき)手(Principal Variation = PV)を通常窓[α,β]で探索したら、残りの手を[α,α+1]でNull Window Search (NWS)します。これでfail highしたらPVを更新するために通常窓で探索し直して、fail lowしたらそのまま放置して良いです。

Negascoutについては、[note](https://note.com/nyanyan_cubetech/n/nf810b043fb78)に詳しく書きました。

### 置換表による枝刈り (Transposition Cutoff)

ゲーム木はほとんど木構造として見なせるのですが、たまに合流する手筋があります。この際に同じ局面を再度探索しては無駄です。そのため、置換表(特殊なハッシュテーブル)に探索結果をメモしておきます。各ノードで探索前に置換表を参照して、値が登録されていればただちにそれを返せば良いです。また、評価値の下限値と上限値を登録しておくと、探索窓を狭める効果も期待できます。

置換表に値が登録されていたとしても、現在行っている探索よりも浅い探索の結果であればそれを採用してはいけません。Egaroucidでは評価値の下限と上限に加えて探索の深さとMulti ProbCutの確率、さらに探索の新しさを記録することで、現在行っている探索の結果として置換表の値を採用するかどうかを決定しています。

なお、置換表の参照にはオーバーヘッドがあるので、それが無視できるほど根に近いノードでのみ置換表へのアクセスを行います。根に近いノードでのみ行うことで、置換表に登録するデータを必要最低限に減らすこともできます。

さらに、置換表に過去の探索での最善手も一緒に記録しておくことで、現在の探索での手の並び替え(Move Ordering)にも使えます。

### 置換表による拡張した枝刈り(Enhanced Transposition Cutoff)

あるノードを探索するとき、そのノードが置換表に登録されていなくても、そのノードの子ノードを展開して置換表を参照すると、探索窓を狭めたり、評価値を確定できる場合があります。これはEdaxにてEnhanced Transposition Cutoff (ETC)として実装されているアイデアです。

あるノードを[α,β]の探索窓で探索するとします。また、子ノードを展開して、各子ノードについて置換表に記載されている(子ノードの手番から見た)最小値Lと最大値Hを見ていきます。すると、$\beta<\max(-\{L\})$であれば$\beta$を$\max(-\{L\})$に更新できますし、同様にして$\alpha>\max(-\{H\})$であれば$\alpha$を$\max(-\{H\})$に更新できます。

これは子ノードを展開した後に置換表の参照を繰り返すため、オーバーヘッドがかなり大きいです。根に近いノードでのみ行います。

### 確定石による枝刈り (Stability Cutoff)

ある盤面について、黒の確定石がB個、白の確定石がW個あれば、最終局面はB対64-Bから64-W対Wのどれかになるので、例えば黒目線での評価値(最終石差)は2B-64から64-2Wの中に入ることがわかります。これを用いて探索窓を狭めることができる場合があります。これはEdaxにてStability Cutoffとして実装されているアイデアです。

この手法は探索しているノードが終局に十分近い場合、つまり確定石が存在する可能性が高い場合にしか使えません。また、確定石の計算にはオーバーヘッドがありますので、あまりにも終局に近すぎると、単純に探索した方が速くなることも多くあります。

なお、確定石の個数が少ないと狭められる探索窓の範囲は-64や+64に近くなります。そのため、確定石を求める前に確定石の個数が少ないと見込まれる場合は、狭める対象の探索窓が-64や64などに近い領域を含んでいる場合のみ行います。確定石が多いと見込まれれば、狭める対象の探索窓が0に近い場合も枝刈りを試みます。確定石の個数の期待値は局面が進むにつれて増えていく傾向があるので、この見込みには深さと探索窓の上限/下限を対応づけたものを使用します。

確定石自体の計算には、

<ul>
    <li>過去の探索での最善手(置換表に登録されている値)</li>
    <li>辺の確定石(事前計算しておき、辺の形を参照して決定する)</li>
    <li>8方向(縦横斜め)全てのラインが他の石で埋まっている石を確定石とする</li>
    <li>確定石に8近傍を囲まれた石を確定石とする(ループで処理する)</li>
</ul>


を行っています。もちろん、これだけでは完璧に全ての確定石を求めることはできません。しかし、オセロAIの枝刈りという文脈ではこれで十分ですし、あまり正確にして遅くなっても意味がありません。

### Multi ProbCut

これは古くからある手法で、Logistelloで採用されたものです。Logistello以降も現在まで様々なオセロAIで採用されています。この手法は特殊な枝刈りで、探索結果が真の値であるという保証がなくなります。

こちらについてはすでに論文や解説が多いため、ここでの解説は省略します。



## 手の並び替え(Move Ordering)

αβ法の流れを汲むゲーム木探索アルゴリズムでは、各ノードにおいて子ノードを有望な順(良さそうな手の順)に並べて、その順番に探索すると枝刈りが発生しやすくなります。この手の並べ替えをMove Orderingと言います。

Egaroucidでは状況に応じて複数のMove Orderingを用意していますが、以下の要素を使っています。

<ul>
    <li>着手するマスの位置(隅なら優先するなど)</li>
    <li>着手後の相手の合法手数(少ないほど優先する)</li>
    <li>着手後の自分の石の空きマスに接している石の個数(少ないほど優先する)</li>
    <li>局面を4分割したときの空きマスの偶奇(オセロにおける偶数理論の近似的な実装)</li>
    <li>着手後、ごく浅い探索で得られた評価値</li>
</ul>


合法手数と空きマスに接している石の数については、オセロのゲーム自体が相手の打てる手を減らしていくことに価値があるゲームであるという背景があります。

また、合法手が少なければ、単純に展開すべき子ノードの数が少なく済むため、そういった貪欲法を続ければ探索に必要な訪問ノード数を減らせると期待できる面もあります。これは速さ優先探索という名前でも知られています。特にNull Window Search (NWS)においてfail highを狙う場合、fail highさえしてしまえば最善手を探索する必要がないため、速さ優先探索が効果を発揮しやすいです。さらにオセロというゲーム自体、相手の打てる手を絞っていくことが本質にあるゲームなので、その点でも速さ優先探索は都合が良いです。



## 置換表(Transposition Table)

置換表はハッシュテーブルとして実装します。しかしゲーム木探索において特殊な点が、必ずしも過去のデータを保持しなければいけないわけではないという点です。置換表にデータがなければまた探索すれば良いだけなので、置換表へのアクセスを高速化することに重点を置き、ハッシュが衝突した場合には過去のデータを書き換えてしまうなどの実装が適しています。

実際には、深い探索は再探索するのが大変なので優先的に残すなどの工夫をしています。また、ハッシュが衝突した場合にはメモリ上で隣の領域(別のハッシュ値が対応しています)が空いていればそこにデータを入れるなどの、軽いハッシュ衝突対策も行います。

ハッシュが衝突した場合に既存データと新データのどちらを残すかという問題ですが、Egaroucidは以下の指標を見て判断しています。上位のものほど優先されます。

<ul>
    <li>探索の新しさ(新しい探索の結果は優先的に残す)</li>
    <li>そこに登録してある局面から先に読んだ手数(読みが深いほど探索が大変なので残す価値がある)</li>
    <li>その局面を探索するときのMulti ProbCutの確率(MPCの確率が高いほど探索が大変で値が正確なので残す価値がある)</li>
</ul>

このような扱いをして、さらに、置換表は基本的にリセットしないですべての探索で使い回すことにしています。




## 並列化

minimax系統のアルゴリズムはαβ法(αβ枝刈り)という手法で大幅な枝刈りを行えますが、これは逐次的に探索することを前提とした枝刈りなので、探索を並列化する場合には枝刈り効率が落ちないように注意する必要があります。

EgaroucidではYBWC(Young Brothers Wait Concept)を用いてCPUを使った並列化を行いました。YBWCは枝刈り効率を下げにくいままに、それなりの効率で並列化できる手法です。詳しい解説は他の人に委ねます。

YBWCはなかなか並列化効率を上げにくく、並列化を進めても手元の環境ではスピードは逐次探索と比べて6-7倍程度で頭打ちになります。現在、打開策を考えています。



## 定数倍高速化

minimax系統のアルゴリズムを使ったオセロAIはアルゴリズムの性質上、深く読むと深さに従って指数的に計算量が増大します。計算量自体を根底から改善することは不可能なので、ここで大事なのが定数倍の高速化です。

Egaroucidでは、ビットボードによる高速化や、SIMDを使った高速化、さらに最終盤の探索は空きマス数に応じて専用関数を使った最適化(最終N手最適化)を行うなどして定数倍高速化に努めました。

SIMDによる高速化は奥原氏によるEdaxのAVXを使った最適化の解説が参考になります。

<ul>
    <li>[ビットボード関連の高速化](http://www.amy.hi-ho.ne.jp/okuhara/bitboard.htm)</li>
    <li>[ビットボード以外の高速化](http://www.amy.hi-ho.ne.jp/okuhara/edaxopt.htm)</li>
</ul>



## 特殊な終局への対策

Egaroucidは序盤で中盤探索を行います。これは終局まで読みきらずに評価関数による評価値を探索結果として使った探索です。

しかし、当然ながら評価関数は完璧ではありません。例えば大量取りされた場合、圧倒的に有利なはずなのに間違って全滅筋に入ってしまうなど、評価関数だけでは見抜けない特殊な終局があります。

例えば以下の局面は白番ですが、白が圧倒的に有利な状況にあります。しかし、間違ってe6に白が置いてしまうと、黒e3で全滅して白は負けてしまいます。

<div class="centering_box">
    <img class="pic2" src="img/clog.png">
</div>

こういった特殊な終局(有利だが一手だけ大悪手があるなど)の回避のため、Egaroucidではある程度の深さの考えうる全部の手を高速に列挙し、こういった特殊な終局が発見されないかを確認してから中盤探索を行っています。純粋にビットボードの処理だけしかしないため、非常に高速に実装できます。



## 最終N手最適化

オセロにおいて特徴的なのは、終盤の完全読みです。中盤探索については評価関数の精度によってミスをする可能性がありますが、完全読みさえしてしまえば、もうミスをする余地は(バグがない限り)一切ありません。

ですので、オセロAIの強さには完全読みの速さが大きく影響します。早い段階で完全読みを行えれば、オセロAIは強くなります。Egaroucidではレベルによって完全読みタイミングを変えていますが、例えばデフォルトのレベル21では24マス空き以降は完全読みを行います。また、レベル21では30マス空きから読み切り(終局まで探索するものの、悪手と思われる手を評価関数によって一部省いてしまう)も行います。

例えば24マス空きの完全読みでは1e7から1e8ほどと、それなりに多くのノードを訪問します。また、ゲーム木はその名の通り木構造ですから、葉に近いノードが非常に多いです。つまり、終局間近の数手だけ専用関数を使って頑張って高速化すれば、完全読み全体を高速化できる見込みがあるのです。

ということで、ここでは最終N手最適化として、Egaroucidで用いている最終1手から4手の最適化手法を解説します。

現実的には終局間近かどうかの判定は難しいので、盤面に空きマスが何マスあるかを見て専用関数への移行を行います。

### 1マス空きの最適化

通常、石数のカウントは盤面に石を置いた後に盤面の石を数えて行いますが、1マス空きについては、もはや盤面に石を置く処理すら不要です。必要なのは、

<ul>
    <li>現在の盤面の石差</li>
    <li>空きマスに着手したときに返る石の数</li>
</ul>

の2つの数だけです。

現在の盤面の石差は、盤面が必ず1マス空きであるという特性を利用すれば、一方のプレイヤの石数をカウントするだけで計算できます。

空きマスに着手したときに返る石の数は、専用関数を作って求めます。空きマスから縦横斜めの4方向についてそれぞれ返る石の数を求めて合計します。このとき、各ラインで返る石の数は事前計算で求めておいて、実際の探索ではラインの抜き出しと表引きで実装しました。

また、現在の盤面の石差を求めた時点で探索窓に比べて明らかに枝刈りできる場合は、返る石の数の計算を省略することも可能です。

なお、返る石の数が0であればパスの処理をしたり、両者置けない場合はそのまま終局にしたりするなどの処理が必要です。ここで、1マス空き(奇数マス空き)で終局するときには引き分けが存在しないことを利用すると、終局の処理を若干高速にできます。

### 2マス空きの最適化

2マス空きでは合法手生成を省いて、簡易的に空きマスの周りに相手の石があれば合法手として返る石を計算し、返る石があれば着手するという処理を行いました。

なお、2マス空きでは偶数理論もどき(手の並び替えの項目を参照)に意味がないため、手の並び替えは行いません。ただ、2連打が確定する手筋があればそちらを優先する(つまり、相手からも置ける可能性のあるマスを優先して探索する)などの工夫ができます。



### 3マス空きの最適化

3マス空きでは、偶数理論もどきによって手の並び替えを行えます。盤面のそれぞれの象限を見て、ある象限には1マス、別の象限では2マスの空きマスがあれば、1マスの空きを優先して着手します。この並べかえでは条件分岐をなくすため、マスの位置(0から63で表す)と象限を対応付けるbitをうまく使って、SIMDのshuffle関数と表引きを使うことで並び替えを実現しています。このアイデアはEdaxをAVXに最適化した[奥原氏の解説](http://www.amy.hi-ho.ne.jp/okuhara/edaxopt.htm)を参考にしました。

3マス空きでも2マス空きと同じように、合法手生成を省いて簡易的に合法手判定をして、順番に着手を試みます。3マス空きで終局する場合は、引き分けが存在しないので引き分けの処理を省いた関数で石差を計算します。



### 4マス空きの最適化

4マス空きでは、偶数理論もどきによって手の並べ替えを行います。象限ごとに1マス-1マス-2マス-0マスの空きがある場合、1マス空きの象限2つを優先して探索します。これも条件分岐をなくすため、3マス空きと同じようにshuffleを用いた最適化を行いました。

4マス空きでも合法手生成を省いて簡易的に合法手判定をして、順番に着手を試みます。


