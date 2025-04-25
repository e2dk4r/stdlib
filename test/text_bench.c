#include <time.h>

#include "print.h"
#include "string_builder.h"
#include "text.h"

// returns current time in nanoseconds
internalfn u64
now(void)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts))
    runtime_assert(0 && "clock is unstable");

  return (u64)ts.tv_sec * 1000000000 /* 1e9 */ + (u64)ts.tv_nsec;
}

internalfn void
StringBuilderAppendDuration(string_builder *sb, struct duration *duration)
{
  struct string nanosecondUnitString = StringFromLiteral("ns");
  struct string microsecondUnitString = StringFromLiteral("us");
  struct string millisecondUnitString = StringFromLiteral("ms");
  struct string secondUnitString = StringFromLiteral("sec");
  struct string minuteUnitString = StringFromLiteral("min");
  struct string hourUnitString = StringFromLiteral("hr");
  struct string dayUnitString = StringFromLiteral("day");
  struct string weekUnitString = StringFromLiteral("wk");

  u64 microsecondInNanoseconds = 1000UL /* 1e3 */;
  u64 millisecondInNanoseconds = 1000000UL /* 1e6 */;
  u64 secondInNanoseconds = 1000000000UL /* 1e9 */;
  u64 minuteInNanoseconds = 1000000000UL /* 1e9 */ * 60;
  u64 hourInNanoseconds = 1000000000UL /* 1e9 */ * 60 * 60;
  u64 dayInNanoseconds = 1000000000UL /* 1e9 */ * 60 * 60 * 24;
  u64 weekInNanoseconds = 1000000000UL /* 1e9 */ * 60 * 60 * 24 * 7;

  u64 remaining = duration->ns;

  if (remaining >= weekInNanoseconds) {
    u64 weeks = remaining / weekInNanoseconds;
    StringBuilderAppendU64(sb, weeks);
    StringBuilderAppendString(sb, &weekUnitString);
    remaining -= weeks * weekInNanoseconds;
  }

  if (remaining >= dayInNanoseconds) {
    u64 days = remaining / dayInNanoseconds;
    StringBuilderAppendU64(sb, days);
    StringBuilderAppendString(sb, &dayUnitString);
    remaining -= days * dayInNanoseconds;
  }

  if (remaining >= hourInNanoseconds) {
    u64 hours = remaining / hourInNanoseconds;
    StringBuilderAppendU64(sb, hours);
    StringBuilderAppendString(sb, &hourUnitString);
    remaining -= hours * hourInNanoseconds;
  }

  if (remaining >= minuteInNanoseconds) {
    u64 minutes = remaining / minuteInNanoseconds;
    StringBuilderAppendU64(sb, minutes);
    StringBuilderAppendString(sb, &minuteUnitString);
    remaining -= minutes * minuteInNanoseconds;
  }

  if (remaining >= secondInNanoseconds) {
    u64 seconds = remaining / secondInNanoseconds;
    StringBuilderAppendU64(sb, seconds);
    StringBuilderAppendString(sb, &secondUnitString);
    remaining -= seconds * secondInNanoseconds;
  }

  if (remaining >= millisecondInNanoseconds) {
    u64 milliseconds = remaining / millisecondInNanoseconds;
    StringBuilderAppendU64(sb, milliseconds);
    StringBuilderAppendString(sb, &millisecondUnitString);
    remaining -= milliseconds * millisecondInNanoseconds;
  }

  if (remaining >= microsecondInNanoseconds) {
    u64 microseconds = remaining / microsecondInNanoseconds;
    StringBuilderAppendU64(sb, microseconds);
    StringBuilderAppendString(sb, &microsecondUnitString);
    remaining -= microseconds * microsecondInNanoseconds;
  }

  if (remaining != 0) {
    u64 nanoseconds = remaining;
    StringBuilderAppendU64(sb, nanoseconds);
    StringBuilderAppendString(sb, &nanosecondUnitString);
  } else {
    StringBuilderAppendStringLiteral(sb, "0");
  }
}

internalfn void
StringBuilderAppendPrintableString(string_builder *sb, struct string *string)
{
  if (string->value == 0)
    StringBuilderAppendStringLiteral(sb, "(NULL)");
  else if (string->value != 0 && string->length == 0)
    StringBuilderAppendStringLiteral(sb, "(EMPTY)");
  else if (string->length == 1 && string->value[0] == ' ')
    StringBuilderAppendStringLiteral(sb, "(SPACE)");
  else
    StringBuilderAppendString(sb, string);
}

int
main(void)
{
  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  struct string *function;

  function = &StringFromLiteral("b8 IsStringEqual(struct string *left, struct string *right)");
  {
    struct string *left =
        &StringFromLiteral("%-xr@{@K0tl|.SjY?+O`;mYFNG)kH(0e~fmER~kOK*Bg+E\"[g^Kg1A#?{k&skQMM@9,6D:`F:8f}cF`u\"l3<mJ=:"
                           "S17~]@||(2fcj\"2eh6U?152];$O$pxaJior:eY<y$.E<7I[!4P7@]&J!Iol2*RK@#x!%H\";9[=PU}/"
                           "ylibL<#LB+T}]d;$E63h|3P4@<;]f2~HDVVO)<gja;Ei|z3@/*yB|IwPim#Ad)u2i>s>5:/"
                           "m2Gv.~|V`R1epP}sOSug!4sNcp~B2'_}MH(:]ZdZ^)wcHLXIP$^8.yI?_=c,lL-bC[7BAu/"
                           "pz~K3TW:?+}LDu5AO`P*etZ1(JjgAKlN~Kd?#8B1.-[]$P6Y55wjwJlnTcUo,(m=0%8&N)xAqVDIG4mb1hK[G@KSf:"
                           "opN5PJ)Li4A_8Y1g$L3xC^zrwR'Q~-'t?$&vDwX2HX9q[s#Xh+`<QWz7$]w.I2&v_p>1f\"2IC}%|=@vH-pf(Q!u_<("
                           "<[Y(\"C[0#>%.T<J+{,R4(og>RaU9](sHzS4^c3y[kc*d,uUTBr&J6*H5MVgxr>Xp&Y\"~PY-}I=Y;OC)/"
                           "xm4a(m',_nw_T'{M<!w=Vi5(&8G7x|tgzfmaY\":gG9M=>&ao[1,16)/N*8=z5YrN6=eaxuXKA@Ss'`(2*9/"
                           "R7tT~D32)1P($LMU1#b#:w41I7=_!oF;ymya}#}Bg>DEhZ-gDpzEMI`2uN=4^:}7Z|MWdsacviRo0GKAjD&'k<"
                           "DUdAaDGs\"gb>mpqLyn;btb(2:i_R%=V3YLF32_Z-t-E]z8t2nTeRc.9<*S-y@G%aS!)i9DC\"M8lb48zLzl:43|d]>"
                           "l85[;xG]^/go~m<CPD?6vy\"ccCu@nGGU6d+8IBv95d%[/5-q#HL*HX0wSFl`bB14#qzmYa1Di<YtW/"
                           ")*\"xN$t2ds0PICZ=T*8{.;B=lv*d|cNK+_y*(@H6H:490biWq.bi};6&=m#h8+dbcb#VNC)g}}2Qa3/"
                           ",%P>R',qZuJ3^\"OlM0RXs'kN]+ik%JRUNp1|3r%d~");
    struct string *right = &StringFromLiteral(
        "$9Ymf[[NP0jJPX>o:]zN'SF%TwJUSCT>0g1\"Qebh)f+rtld$:\"[`2~3{[:dqM[zW)L>4L`<{!0LvhX,LUqO2J\"C*M.^r@M6p-pef<rwu*P="
        "KYg'cJ(a8z7-*?Lh4szB*ojJp9V';26R=c/"
        "IP@zPGZ_XV3$2^&Hwa'&kk)Ys|Y+KcEyRV6iT#W{<7cHVRC7VLrFnhXHF>Q4a]OeO%6h|;bL7@U3>XK(.'0RPMKT';R;:{,{0;^e#/"
        "@_T>G?z8N~c9(OX5KOU7z<S^0klnowDs!>u?:wJMR2~i$!n##<*'t=8l_4w=q:ds>Z%Kz&<cqm0[&QKo9c9]lrw#R~6}x;|Flx,LD7:$4M`dD<"
        "N)*Am%4K@DN4jVgs/"
        "Y9!WvNTg;c\"LHA8&-JRG6GC{y[~zKbZ^;g^:{27%k~J^4I@(]1|H9&EHDEK[nX,K_x,jDL.f!YuEQr@S@S3u\"p)+G\"z~ygb{oq8*"
        "v\"1qbq`qz/$$e+|Yn$Yrr7\":N]0qTT|u_QuS8``v%uS%A.<$k/"
        "E_h&o9|Q,ek:<ErJv9[3YK4zUXB17t\"C$^!24i^_7w#OFfE=lu<:cZ6$<e6e?$}^3O3}9n{j:3J6,R5y6pF8z_N<\"mfk.6^ej<^N;|10\"7="
        "PY@*hmFc!=pq#`93[~o!Z$+)f;i{=W6WxA)-!8Q;NU$l2HnJvzrnz?Tnhz7n~q:ia@T~!#J1XrBh:KWo.l\"vsHL$*^2s1VQlRg\"2tCd#ou`;"
        "W~e`Q:f<ozShkyuuJq=a`Vp9?o#q*Q1]X}(h0oVA^7dtwR`JJOmpB)7C^x;WeK>;8t^|f0!5-*.T#9&Su36;f'NY)G02$8+!qnD-/"
        "<;<Ndz,#cISvdQ}Oe^(_F6(s4oa2+fWbNFVuV*ABwqFlEgAL]?uuF9t-YE[/"
        "8)PAP{CWc}#I%*6Clie@y`H1'Xz$(#S\"Nl_0xL'@+!]@]{)%`Ithu.aAsN%s7o,LOg[GA\"7ndD6?k6CWACW7J0JI!^uEj\"M'o(D~Z~o^~"
        "JFeQ*G]zEb)`:#Qc!yhb3V");
    u64 iterations = 10000000;
    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      IsStringEqual(left, right);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  // b8 IsStringEqualIgnoreCase(struct string *left, struct string *right)
  {
  }

  // IsStringContains(struct string *string, struct string *search)
  {
  }

  // IsStringStartsWith(struct string *string, struct string *search)
  {
  }

  // b8 IsStringEndsWith(struct string *string, struct string *search)
  {
  }

  // struct string StringStripWhitespace(struct string *string)
  {
  }

  function = &StringFromLiteral("b8 ParseDuration(struct string *string, struct duration *duration)");
  {
    struct string *input = &StringFromLiteral("78wk46day27hr08min14sec");
    u64 iterations = 1000000;
    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      struct duration duration;
      ParseDuration(input, &duration);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  // IsDurationLessThan(struct duration *left, struct duration *right)
  // IsDurationGraterThan(struct duration *left, struct duration *right)
  {
  }

  function = &StringFromLiteral("b8 ParseU64(struct string *string, u64 *value)");
  {
    struct string *input = &StringFromLiteral("11347919234869594277");
    u64 iterations = 1000000;
    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      u64 value;
      ParseU64(input, &value);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  function = &StringFromLiteral("struct string FormatU64(struct string *stringBuffer, u64 value)");
  {
    u64 input = 5057023407986315;
    u64 iterations = 1000000;

    u8 buffer[16];
    struct string stringBuffer = {
        .value = buffer,
        .length = ARRAY_COUNT(buffer),
    };

    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      FormatU64(&stringBuffer, input);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  function = &StringFromLiteral("b8 ParseHex(struct string *string, u64 *value)");
  {
    struct string *input = &StringFromLiteral("d6170a8bea");
    u64 iterations = 1000000;
    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      u64 value;
      ParseHex(input, &value);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  function = &StringFromLiteral("struct string FormatHex(struct string *stringBuffer, u64 value)");
  {
    u64 input = 0x01bfb0971479c1f0;
    u64 iterations = 1000000;

    u8 buffer[16];
    struct string stringBuffer = {
        .value = buffer,
        .length = ARRAY_COUNT(buffer),
    };

    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      FormatHex(&stringBuffer, input);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  function =
      &StringFromLiteral("struct string FormatF32Slow(struct string *stringBuffer, f32 value, u32 fractionCount)");
  {
    f32 input = 314.0717f;
    u64 iterations = 1000000;

    u8 buffer[8];
    struct string stringBuffer = {
        .value = buffer,
        .length = ARRAY_COUNT(buffer),
    };

    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      FormatF32Slow(&stringBuffer, input, 4);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  function = &StringFromLiteral("struct string FormatF32(struct string *stringBuffer, f32 value, u32 fractionCount)");
  {
    f32 input = 314.0717f;
    u64 iterations = 1000000;

    u8 buffer[8];
    struct string stringBuffer = {
        .value = buffer,
        .length = ARRAY_COUNT(buffer),
    };

    u64 start = now();
    for (u64 iteration = 0; iteration < iterations; iteration++) {
      FormatF32(&stringBuffer, input, 4);
    }
    struct duration elapsed = DurationBetweenNanoseconds(start, now());
    StringBuilderAppendStringLiteral(sb, "  function: ");
    StringBuilderAppendString(sb, function);
    StringBuilderAppendStringLiteral(sb, "\niterations: ");
    StringBuilderAppendU64(sb, iterations);
    StringBuilderAppendStringLiteral(sb, "\n   elapsed: ");
    StringBuilderAppendDuration(sb, &elapsed);
    StringBuilderAppendStringLiteral(sb, "\n");
    struct string message = StringBuilderFlush(sb);
    PrintString(&message);
  }

  StringBuilderAppendStringLiteral(sb, "----------------------------------------------------------------\n");

  // struct string PathGetDirectory(struct string *path)
  {
  }

  // b8 StringSplit(struct string *string, struct string *separator, u64 *splitCount, struct string *splits)
  {
  }

  return 0;
}
