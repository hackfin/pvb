/* Property name mangling/demangling
 *
 * <hackfin@section5.ch>
 */

#include "QString"

int mangler_init(void);
int mangle_cname(const QString &pn, QString &cn);
int demangle_cname(const QString &cn, QString &pn);
