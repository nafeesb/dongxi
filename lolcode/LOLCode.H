/*! \file LOLCode.H
  \author Nafees Bin Zafar <nafees@d2.com>
 */
#ifndef DD_LOLCODE_H
#define DD_LOLCODE_H

#define GIMMEH  std::cin >>
#define VISIBLE std::cout <<

#define MOAR ++
#define NERF --

#define BIGR_THAN  >
#define SMALR_THAN >
#define LIEK       ==

#define HAI         int main(int argc, char** argv) {
#define KTHXBYE     return 0; }
#define KTHX    }

#ifdef _MSC_VER
/* windows */
#  define BTW(msg) __pragma(message(msg));
#else
/* at least gcc */
#  define BTW(msg) _Pragma("info msg")
#endif

#define PLZ try {
#define O_NOES(pred) } catch ( pred ) {
#define DO_NOT_WANT    throw

#define I_ARE(cname) class cname {

#define I_MADE_YOU_A   new
#define BUT_I_EATED_IT delete

#endif
