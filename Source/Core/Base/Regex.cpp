#include "CoreMinimal.h"

#define PCRE2_STATIC 1
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace k3d
{
    class MatchBase
    {
    public:
        MatchBase(pcre2_real_code_8* InRE)
            : m_RE(InRE)
        {
            m_Data = pcre2_match_data_create_from_pattern(m_RE, NULL);
        }
        virtual ~MatchBase()
        {
            if (m_Data)
            {
                pcre2_match_data_free(m_Data);
                m_Data = nullptr;
            }
        }
        bool Search(const char* Str)
        {
            int rc = pcre2_match(m_RE, (PCRE2_SPTR)Str, strlen(Str),
                0,                      /* starting offset in the subject */
                0,                      /* options */
                m_Data,
                NULL);                  /* use default match context */
            if (rc < 0)
            {
                return false;
            }
            return rc > 0;
        }
    protected:
        pcre2_real_code_8*          m_RE;
        pcre2_real_match_data_8*    m_Data;
    };

    class MatchGroup : public MatchBase
    {
    public:
        MatchGroup(pcre2_real_code_8* InRe, RegEx::Groups& OutGroup);
        ~MatchGroup();

        bool Match(const char* Str);

    private:
        RegEx::Groups&              m_Groups;
    };

    RegEx::RegEx(const char * Expr, Option Mode)
        : m_Code(nullptr)
        , m_Mode(Mode)
        , m_CompileErrorNo(0)
        , m_CompileErrorOffset(0)
    {
        /*
        if (opt & RegEx::IgnoreCase)
        {
            m_Flags |= PCRE2_CASELESS;
        }*/
        m_Code = pcre2_compile(
            (PCRE2_SPTR)Expr, PCRE2_ZERO_TERMINATED,
            PCRE2_UTF,
            &m_CompileErrorNo, &m_CompileErrorOffset,
            NULL);
    }

    RegEx::~RegEx()
    {
        if (m_Code)
        {
            pcre2_code_free(m_Code);
            m_Code = nullptr;
        }
    }

    bool RegEx::IsValid() const
    {
        return m_Code != nullptr;
    }

    String RegEx::GetError() const
    {
        PCRE2_UCHAR buffer[256] = { 0 };
        pcre2_get_error_message(m_CompileErrorNo, buffer, sizeof(buffer));
        return String::Format("PCRE2 compilation failed at offset %d: %s\n", (int)m_CompileErrorOffset, buffer);
    }

    bool RegEx::Match(const char * Str, Option option)
    {
        MatchBase match(m_Code);
        return match.Search(Str);
    }

    bool RegEx::Match(const char * Str, Groups& OutGroups, Option InOption)
    {
        MatchGroup match(m_Code, OutGroups);
        return match.Match(Str);
    }

    String RegEx::Group::SubGroup(int Id) const
    {
        auto Elem = m_SubNonNamedGroups[Id];
        return String(m_Ptr+Elem.Start, Elem.Length);
    }

    String RegEx::Group::SubGroup(const char * Name) const
    {
        int i = 0;
        for (; i < m_SubNamedGroups.Count(); i++)
        {
            if (!strcmp(m_SubNamedGroups[i].Name.Data(),Name))
                break;
        }
        return String(m_Ptr + m_SubNamedGroups[i].Start, m_SubNamedGroups[i].Length);
    }

    MatchGroup::MatchGroup(pcre2_real_code_8 * InRe, RegEx::Groups& OutGroup)
        : MatchBase(InRe)
        , m_Groups(OutGroup)
    {
    }

    MatchGroup::~MatchGroup()
    {
    }

    bool MatchGroup::Match(const char * Str)
    {
        int rc = pcre2_match(m_RE, (PCRE2_SPTR)Str, strlen(Str),
            0,                  /* starting offset in the subject */
            PCRE2_ANCHORED,                  /* options */
            m_Data,             /* block for storing the result */
            NULL);              /* use default match context */

        if (rc <= 0)
            return false;

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(m_Data);
        int namecount = 0;
        pcre2_pattern_info(m_RE, PCRE2_INFO_NAMECOUNT, &namecount);
        
        RegEx::Group group(rc, Str);
        if (namecount > 0)
        {
            int name_entry_size = 0;
            PCRE2_SPTR tabptr;
            pcre2_pattern_info(m_RE, PCRE2_INFO_NAMETABLE, &tabptr);
            pcre2_pattern_info(m_RE, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);
            for (int i = 0; i < namecount; i++)
            {
                int index = (tabptr[0] << 8) | tabptr[1];
                int name_len = name_entry_size - 3;
                PCRE2_SPTR name_ptr = tabptr + 2;
                int value_len = (int)(ovector[2 * index + 1] - ovector[2 * index]);
                PCRE2_SPTR value_ptr = (PCRE2_SPTR)Str + ovector[2 * index];
                tabptr += name_entry_size;
                RegEx::GroupElement element;
                element.Name = Move(String(name_ptr, name_len));
                element.Index = index;
                element.Start = ovector[2 * index];
                element.Length = value_len;
                group.SetType(index, RegEx::Group::Named);
                group.AppendNamedElement(Move(element));
            }
        }

        for (int i = 1; i < rc; i++)
        {
            if (!group.IsNamed(i))
            {
                RegEx::GroupElement element;
                element.Index = i;
                element.Start = ovector[2 * i];
                element.Length = ovector[2 * i + 1] - ovector[2 * i];
                group.AppendNonNamedElement(Move(element));
            }
        }
        m_Groups.Append(group);
        return true;
    }

}