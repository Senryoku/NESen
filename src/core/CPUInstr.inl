////////////////////////////////////////////////////////////////////////////////////////////////
// Instruction Set
// All addresses are absolute here.

// In alphabetical order.

inline void adc(addr_t addr) {
    _adc(read(addr));
}

inline void _adc(word_t operand) {
    const uint16_t add = _reg_acc + operand + (_reg_ps & StateMask::Carry);

    set(StateMask::Carry, add > 0xFF);

    // Overflow is set if the sum of two inputs with the same sign
    // produce a result with a different sign.
    // a ^ b	=> 1 if sign bits are differents
    // & 0x80	=> Conserve only sign bit
    set(StateMask::Overflow, (~(_reg_acc ^ operand) & (_reg_acc ^ add)) & 0x80);

    _reg_acc = static_cast<word_t>(add & 0xFF);

    set_neg_zero(_reg_acc);
}

inline void and_(addr_t addr) {
    _reg_acc &= read(addr);
    set_neg_zero(_reg_acc);
}

inline word_t asl(addr_t addr) {
    return asl(read(addr));
}

inline word_t asl(word_t operand) {
    set(StateMask::Carry, operand & 0b10000000);
    operand = operand << 1;
    set_neg_zero(operand);
    return operand;
}

// Helper
inline void relative_jump(bool b) {
    int offset = read(_reg_pc++);
    if(b)
        _reg_pc += from_2c_to_signed(offset);
}

inline void bcc() {
    relative_jump(!check(StateMask::Carry));
}

inline void bcs() {
    relative_jump(check(StateMask::Carry));
}

inline void beq() {
    relative_jump(check(StateMask::Zero));
}

inline void bit(addr_t addr) {
    word_t tmp = (_reg_acc & read(addr));
    set(StateMask::Negative, read(addr) & StateMask::Negative);
    set(StateMask::Overflow, read(addr) & StateMask::Overflow);
    set(StateMask::Zero, tmp == 0);
}

inline void bmi() {
    relative_jump(check(StateMask::Negative));
}

inline void bne() {
    relative_jump(!check(StateMask::Zero));
}

inline void bpl() {
    relative_jump(!check(StateMask::Negative));
}

inline void brk() {
    _reg_pc++;
    push16(_reg_pc);
    push(_reg_ps | 0b00110000);
    _reg_ps |= Interrupt;
    _reg_pc = read16(0xFFFE);
    Log::info("BRK caused a jump to ", Hexa(_reg_pc));
}

inline void bvc() {
    relative_jump(!check(StateMask::Overflow));
}

inline void bvs() {
    relative_jump(check(StateMask::Overflow));
}

inline void clc() {
    clear(StateMask::Carry);
}

inline void cld() {
    clear(StateMask::Decimal);
}

inline void cli() {
    clear(StateMask::Interrupt);
}

inline void clv() {
    clear(StateMask::Overflow);
}

// Helper
inline void comp(word_t lhs, word_t rhs) {
    set(StateMask::Carry, (lhs >= rhs));
    set_neg_zero(lhs - rhs);
}

inline void cmp(addr_t addr) {
    comp(_reg_acc, read(addr));
}

inline void cpx(addr_t addr) {
    comp(_reg_x, read(addr));
}

inline void cpy(addr_t addr) {
    comp(_reg_y, read(addr));
}

inline void dcp(addr_t addr) {
    word_t v = read(addr);
    set(StateMask::Carry, v == 0); /// @todo check
    write(addr, v - 1);
}

inline word_t dec(addr_t addr) {
    word_t operand = read(addr);
    --operand;
    set_neg_zero(operand);
    return operand;
}

inline void dex() {
    set_neg_zero(--_reg_x);
}

inline void dey() {
    set_neg_zero(--_reg_y);
}

inline void eor(addr_t addr) {
    _reg_acc ^= read(addr);
    set_neg_zero(_reg_acc);
}

inline word_t inc(addr_t addr) {
    word_t operand = read(addr);
    ++operand;
    set_neg_zero(operand);
    return operand;
}

inline void inx() {
    ++_reg_x;
    set_neg_zero(_reg_x);
}

inline void iny() {
    ++_reg_y;
    set_neg_zero(_reg_y);
}

inline void jmp(addr_t addr) {
    _reg_pc = addr;
}

inline void jsr(addr_t addr) {
    push16(_reg_pc - 1);
    _reg_pc = addr;
}

inline void lax(addr_t addr) {
    word_t operand = read(addr);
    set_neg_zero(operand);
    _reg_acc = operand;
    _reg_x = operand;
}

inline void lda(addr_t addr) {
    word_t operand = read(addr);
    set_neg_zero(operand);
    _reg_acc = operand;
}

inline void ldx(addr_t addr) {
    auto operand = read(addr);
    set_neg_zero(operand);
    _reg_x = operand;
}

inline void ldy(addr_t addr) {
    word_t operand = read(addr);
    set_neg_zero(operand);
    _reg_y = operand;
}

inline word_t lsr(addr_t addr) {
    return lsr(read(addr));
}

inline word_t lsr(word_t operand) {
    clear(StateMask::Negative);
    set(StateMask::Carry, operand & 0b00000001);
    operand = (operand >> 1) & 0b01111111;
    set(StateMask::Zero, operand == 0);
    return operand;
}

inline void nop() {}

inline void ora(addr_t addr) {
    _reg_acc |= read(addr);
    set_neg_zero(_reg_acc);
}

inline void pha() {
    push(_reg_acc);
}

inline void php() {
    push(_reg_ps | 0b00110000);
}

inline void pla() {
    _reg_acc = pop();
    set_neg_zero(_reg_acc);
}

inline void plp() {
    _reg_ps = pop() & 0b11001111;
}

inline void rla(addr_t addr) {
    rla(read(addr));
}

inline void rla(word_t operand) {
    word_t tmp = rol(operand);
    _reg_acc &= tmp;
}

inline word_t rol(addr_t addr) {
    return rol(read(addr));
}

inline word_t rol(word_t operand) {
    word_t t = operand & 0b10000000;
    operand = (operand << 1) & 0xFE;
    operand = operand | (check(StateMask::Carry) ? StateMask::Carry : 0);
    set(StateMask::Carry, t != 0);
    set_neg_zero(operand);
    return operand;
}

inline word_t ror(addr_t addr) {
    return ror(read(addr));
}

inline word_t ror(word_t operand) {
    word_t t = operand & 0b00000001;
    operand = (operand >> 1) & 0x7F;
    operand = operand | (check(StateMask::Carry) ? 0x80 : 0);
    set(StateMask::Carry, t != 0);
    set_neg_zero(operand);
    return operand;
}

inline void rti() {
    _reg_ps = pop() & 0b11001111;
    _reg_pc = pop16();
}

inline void rts() {
    _reg_pc = pop16() + 1;
}

inline void sax(addr_t addr) {
    /// @todo: check
    write(addr, (_reg_x & _reg_acc));
}

inline void sbc(addr_t addr) {
    _adc(static_cast<word_t>(~read(addr)));
}

inline void sec() {
    set(StateMask::Carry);
}

inline void sed() {
    set(StateMask::Decimal);
}

inline void sei() {
    set(StateMask::Interrupt);
}

inline void sta(addr_t addr) {
    write(addr, _reg_acc);
}

inline void stx(addr_t addr) {
    write(addr, _reg_x);
}

inline void sty(addr_t addr) {
    write(addr, _reg_y);
}

inline void tax() {
    set_neg_zero(_reg_acc);
    _reg_x = _reg_acc;
}

inline void tay() {
    set_neg_zero(_reg_acc);
    _reg_y = _reg_acc;
}

inline void tsx() {
    set_neg_zero(_reg_sp);
    _reg_x = _reg_sp;
}

inline void txa() {
    set_neg_zero(_reg_x);
    _reg_acc = _reg_x;
}

inline void txs() {
    _reg_sp = _reg_x;
}

inline void tya() {
    set_neg_zero(_reg_y);
    _reg_acc = _reg_y;
}

inline void isc(addr_t addr) {
    word_t v = read(addr) + 1;
    write(addr, v);
    sbc(addr);
}
