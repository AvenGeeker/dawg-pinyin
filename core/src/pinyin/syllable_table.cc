// SPDX-License-Identifier: Apache-2.0
#include "pinyin/syllable_table.h"

#include <array>
#include <cstring>

namespace dawg {

namespace {

constexpr std::array<const char*, 390> kSyllables = {{
    // Zero-initial vowels.
    "a", "ai", "an", "ang", "ao", "e", "ei", "en", "eng", "er", "o", "ou",
    // b
    "ba", "bai", "ban", "bang", "bao", "bei", "ben", "beng", "bi", "bian",
    "biao", "bie", "bin", "bing", "bo", "bu",
    // c
    "ca", "cai", "can", "cang", "cao", "ce", "cen", "ceng", "ci", "cong",
    "cou", "cu", "cuan", "cui", "cun", "cuo",
    // d
    "da", "dai", "dan", "dang", "dao", "de", "deng", "di", "dia", "dian",
    "diao", "die", "ding", "diu", "dong", "dou", "du", "duan", "dui", "dun",
    "duo",
    // f
    "fa", "fan", "fang", "fei", "fen", "feng", "fo", "fou", "fu",
    // g
    "ga", "gai", "gan", "gang", "gao", "ge", "gei", "gen", "geng", "gong",
    "gou", "gu", "gua", "guai", "guan", "guang", "gui", "gun", "guo",
    // h
    "ha", "hai", "han", "hang", "hao", "he", "hei", "hen", "heng", "hong",
    "hou", "hu", "hua", "huai", "huan", "huang", "hui", "hun", "huo",
    // j
    "ji", "jia", "jian", "jiang", "jiao", "jie", "jin", "jing", "jiong",
    "jiu", "ju", "juan", "jue", "jun",
    // k
    "ka", "kai", "kan", "kang", "kao", "ke", "kei", "ken", "keng", "kong",
    "kou", "ku", "kua", "kuai", "kuan", "kuang", "kui", "kun", "kuo",
    // l
    "la", "lai", "lan", "lang", "lao", "le", "lei", "leng", "li", "lia",
    "lian", "liang", "liao", "lie", "lin", "ling", "liu", "lo", "long",
    "lou", "lu", "luan", "lun", "luo", "lv", "lve",
    // m
    "ma", "mai", "man", "mang", "mao", "me", "mei", "men", "meng", "mi",
    "mian", "miao", "mie", "min", "ming", "miu", "mo", "mou", "mu",
    // n
    "na", "nai", "nan", "nang", "nao", "ne", "nei", "nen", "neng", "ni",
    "nian", "niang", "niao", "nie", "nin", "ning", "niu", "nong", "nou",
    "nu", "nuan", "nun", "nuo", "nv", "nve",
    // p
    "pa", "pai", "pan", "pang", "pao", "pei", "pen", "peng", "pi", "pian",
    "piao", "pie", "pin", "ping", "po", "pou", "pu",
    // q
    "qi", "qia", "qian", "qiang", "qiao", "qie", "qin", "qing", "qiong",
    "qiu", "qu", "quan", "que", "qun",
    // r
    "ran", "rang", "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru",
    "ruan", "rui", "run", "ruo",
    // s
    "sa", "sai", "san", "sang", "sao", "se", "sen", "seng", "si", "song",
    "sou", "su", "suan", "sui", "sun", "suo",
    // sh
    "sha", "shai", "shan", "shang", "shao", "she", "shei", "shen", "sheng",
    "shi", "shou", "shu", "shua", "shuai", "shuan", "shuang", "shui", "shun",
    "shuo",
    // t
    "ta", "tai", "tan", "tang", "tao", "te", "tei", "teng", "ti", "tian",
    "tiao", "tie", "ting", "tong", "tou", "tu", "tuan", "tui", "tun", "tuo",
    // w
    "wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo", "wu",
    // x
    "xi", "xia", "xian", "xiang", "xiao", "xie", "xin", "xing", "xiong",
    "xiu", "xu", "xuan", "xue", "xun",
    // y
    "ya", "yan", "yang", "yao", "ye", "yi", "yin", "ying", "yo", "yong",
    "you", "yu", "yuan", "yue", "yun",
    // z
    "za", "zai", "zan", "zang", "zao", "ze", "zei", "zen", "zeng", "zi",
    "zong", "zou", "zu", "zuan", "zui", "zun", "zuo",
    // zh
    "zha", "zhai", "zhan", "zhang", "zhao", "zhe", "zhei", "zhen", "zheng",
    "zhi", "zhong", "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang",
    "zhui", "zhun", "zhuo",
}};

}  // namespace

SyllableTable::SyllableTable() {
  syllables_.reserve(kSyllables.size());
  for (const char* s : kSyllables) syllables_.push_back(s);
  by_initial_.resize(26);
  for (int id = 0; id < static_cast<int>(syllables_.size()); ++id) {
    char c = syllables_[id][0];
    if (c >= 'a' && c <= 'z') by_initial_[c - 'a'].push_back(id);
  }
}

const SyllableTable& SyllableTable::Instance() {
  static const SyllableTable table;
  return table;
}

int SyllableTable::Find(std::string_view spelling) const {
  if (spelling.empty() || spelling.front() < 'a' || spelling.front() > 'z') {
    return -1;
  }
  for (int id : by_initial_[spelling.front() - 'a']) {
    if (syllables_[id] == spelling) return id;
  }
  return -1;
}

const std::vector<int>& SyllableTable::ByInitial(char initial) const {
  static const std::vector<int> kEmpty;
  if (initial < 'a' || initial > 'z') return kEmpty;
  return by_initial_[initial - 'a'];
}

bool SyllableTable::IsValidInitial(char initial) const {
  return initial >= 'a' && initial <= 'z' &&
         !by_initial_[initial - 'a'].empty();
}

}  // namespace dawg
